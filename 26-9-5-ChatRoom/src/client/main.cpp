#include "chat/logger.h"
#include "chat/protocol.h"
#include "chat/udp_socket.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace {

bool parsePort(const char* value, std::uint16_t& port) {
    char* end = nullptr;
    const unsigned long number = std::strtoul(value, &end, 10);
    if (end == value || *end != '\0' || number == 0 || number > 65535) {
        return false;
    }
    port = static_cast<std::uint16_t>(number);
    return true;
}

class ChatClient {
public:
    bool start(const std::string& serverIp, std::uint16_t port, std::string nickname) {
        if (nickname.empty() || nickname.size() > chat::kMaxNicknameBytes) {
            std::cerr << "nickname must be 1-20 bytes\n";
            return false;
        }
        if (!chat::parseIpv4Address(serverIp, port, serverAddress_)) {
            std::cerr << "invalid IPv4 address: " << serverIp << '\n';
            return false;
        }
        nickname_ = std::move(nickname);
        // Binding port 0 asks the OS for an available ephemeral port. The same socket
        // is then used by both the sender and receiver thread.
        if (!socket_.bindAny(0)) {
            return false;
        }
        socket_.setDatagramHandler([this](chat::Datagram datagram) { handleDatagram(std::move(datagram)); });
        running_.store(true);
        receiver_ = std::thread([this] { socket_.receiveLoop(running_); });
        send(chat::MessageType::Join, "");
        return true;
    }

    void run() {
        std::string line;
        bool leaveSent = false;
        while (running_.load() && std::getline(std::cin, line)) {
            if (line == "/quit") {
                send(chat::MessageType::Leave, "");
                leaveSent = true;
                break;
            }
            if (line.empty()) {
                continue;
            }
            if (line.size() > chat::kMaxContentBytes) {
                print("message is longer than 512 bytes");
                continue;
            }
            send(chat::MessageType::Chat, line);
        }
        if (!leaveSent && running_.load()) {
            // EOF (for example Ctrl-D) is also a normal client departure from the server's view.
            send(chat::MessageType::Leave, "");
        }
        stop();
    }

    void stop() {
        if (!running_.exchange(false)) {
            return;
        }
        if (receiver_.joinable()) {
            receiver_.join();
        }
        socket_.close();
    }

private:
    void send(chat::MessageType type, const std::string& content) {
        const chat::Message message{type, nickname_, content};
        if (!socket_.sendTo(chat::serializeMessage(message), serverAddress_)) {
            print("failed to send message");
        }
    }

    void handleDatagram(chat::Datagram datagram) {
        std::string error;
        const auto message = chat::parseMessage(datagram.payload, error);
        if (!message.has_value()) {
            print("received invalid server message: " + error);
            return;
        }
        if (message->type == chat::MessageType::Chat) {
            print(message->nickname + ": " + message->content);
        } else if (message->type == chat::MessageType::System) {
            print("[system] " + message->content);
            if (message->content == "JOIN_OK") {
                print("connected; type /quit to leave");
            }
        }
    }

    void print(const std::string& text) {
        std::lock_guard<std::mutex> lock(outputMutex_);
        std::cout << text << '\n';
    }

    chat::UdpSocket socket_;
    sockaddr_in serverAddress_{};
    std::string nickname_;
    std::atomic_bool running_{false};
    std::thread receiver_;
    std::mutex outputMutex_;
};

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "usage: " << argv[0] << " <server_ip> <nickname> [port]\n";
        return EXIT_FAILURE;
    }

    std::uint16_t port = 9000;
    if (argc == 4 && !parsePort(argv[3], port)) {
        std::cerr << "invalid port\n";
        return EXIT_FAILURE;
    }

    ChatClient client;
    if (!client.start(argv[1], port, argv[2])) {
        return EXIT_FAILURE;
    }
    client.run();
    return EXIT_SUCCESS;
}
