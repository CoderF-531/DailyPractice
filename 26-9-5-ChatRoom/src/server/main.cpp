#include "chat/logger.h"
#include "chat/protocol.h"
#include "chat/thread_pool.h"
#include "chat/udp_socket.h"

#include <arpa/inet.h>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using chat::Datagram;

std::atomic_bool g_running{true};

void handleSignal(int) {
    g_running.store(false);
}

bool parsePort(const char* value, std::uint16_t& port) {
    char* end = nullptr;
    const unsigned long number = std::strtoul(value, &end, 10);
    if (end == value || *end != '\0' || number == 0 || number > 65535) {
        return false;
    }
    port = static_cast<std::uint16_t>(number);
    return true;
}

struct ClientInfo {
    std::string nickname;
    sockaddr_in address{};
};

class ChatServer {
public:
    bool start(std::uint16_t port) {
        if (!socket_.bindAny(port)) {
            return false;
        }
        socket_.setDatagramHandler([this](Datagram datagram) {
            // The datagram owns its copied payload, so this asynchronous task never observes
            // the receive loop's reusable buffer.
            chat::ThreadPool::instance().submit([this, datagram = std::move(datagram)]() mutable {
                handleDatagram(std::move(datagram));
            });
        });
        chat::Logger::info("chat server listening on 0.0.0.0:" + std::to_string(port));
        return true;
    }

    void run() {
        socket_.receiveLoop(g_running);
        socket_.close();
        chat::ThreadPool::instance().stop();
    }

private:
    static std::string clientKey(const sockaddr_in& address) {
        return chat::formatAddress(address);
    }

    void handleDatagram(Datagram datagram) {
        std::string error;
        const auto message = chat::parseMessage(datagram.payload, error);
        if (!message.has_value()) {
            chat::Logger::warn("rejected datagram from " + clientKey(datagram.peer) + ": " + error);
            sendSystem(datagram.peer, "invalid message: " + error);
            return;
        }

        switch (message->type) {
            case chat::MessageType::Join:
                handleJoin(*message, datagram.peer);
                break;
            case chat::MessageType::Chat:
                handleChat(*message, datagram.peer);
                break;
            case chat::MessageType::Leave:
                handleLeave(*message, datagram.peer);
                break;
            case chat::MessageType::System:
                sendSystem(datagram.peer, "SYSTEM messages are server-only");
                break;
        }
    }

    void handleJoin(const chat::Message& message, const sockaddr_in& peer) {
        std::vector<ClientInfo> recipients;
        std::string rejection;
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            const std::string key = clientKey(peer);
            if (clients_.find(key) != clients_.end()) {
                rejection = "client address is already online";
            } else {
                for (const auto& [_, client] : clients_) {
                    if (client.nickname == message.nickname) {
                        rejection = "nickname is already in use";
                        break;
                    }
                    recipients.push_back(client);
                }
                if (rejection.empty()) {
                    clients_.emplace(key, ClientInfo{message.nickname, peer});
                }
            }
        }

        if (!rejection.empty()) {
            sendSystem(peer, rejection);
            return;
        }

        sendSystem(peer, "JOIN_OK");
        for (const ClientInfo& recipient : recipients) {
            sendSystem(recipient.address, message.nickname + " joined the chat");
        }
        chat::Logger::info(message.nickname + " joined from " + clientKey(peer));
    }

    void handleChat(const chat::Message& message, const sockaddr_in& peer) {
        std::vector<ClientInfo> recipients;
        std::string nickname;
        bool joined = true;
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            const auto iterator = clients_.find(clientKey(peer));
            if (iterator == clients_.end()) {
                joined = false;
            } else {
                nickname = iterator->second.nickname;
                for (const auto& [_, client] : clients_) {
                    if (client.address.sin_addr.s_addr != peer.sin_addr.s_addr ||
                        client.address.sin_port != peer.sin_port) {
                        recipients.push_back(client);
                    }
                }
            }
        }

        if (!joined) {
            sendSystem(peer, "join the chat before sending messages");
            return;
        }

        const chat::Message outgoing{chat::MessageType::Chat, nickname, message.content};
        const std::string wire = chat::serializeMessage(outgoing);
        for (const ClientInfo& recipient : recipients) {
            socket_.sendTo(wire, recipient.address);
        }
    }

    void handleLeave(const chat::Message& message, const sockaddr_in& peer) {
        std::vector<ClientInfo> recipients;
        std::string nickname;
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            const auto iterator = clients_.find(clientKey(peer));
            if (iterator == clients_.end()) {
                return;
            }
            nickname = iterator->second.nickname;
            clients_.erase(iterator);
            for (const auto& [_, client] : clients_) {
                recipients.push_back(client);
            }
        }

        for (const ClientInfo& recipient : recipients) {
            sendSystem(recipient.address, nickname + " left the chat");
        }
        chat::Logger::info(nickname + " left from " + clientKey(peer));
        (void)message;
    }

    void sendSystem(const sockaddr_in& peer, const std::string& content) {
        socket_.sendTo(chat::serializeMessage({chat::MessageType::System, "server", content}), peer);
    }

    chat::UdpSocket socket_;
    std::mutex clientsMutex_;
    std::unordered_map<std::string, ClientInfo> clients_;
};

}  // namespace

int main(int argc, char* argv[]) {
    std::uint16_t port = 9000;
    if (argc > 2 || (argc == 2 && !parsePort(argv[1], port))) {
        std::cerr << "usage: " << argv[0] << " [port]\n";
        return EXIT_FAILURE;
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    ChatServer server;
    if (!server.start(port)) {
        return EXIT_FAILURE;
    }
    server.run();
    return EXIT_SUCCESS;
}
