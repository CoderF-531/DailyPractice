#include "chat/udp_socket.h"

#include "chat/logger.h"
#include "chat/protocol.h"

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <sys/socket.h>
#include <sys/time.h>
#include <utility>
#include <unistd.h>

namespace chat {

UdpSocket::~UdpSocket() {
    close();
}

bool UdpSocket::open() {
    std::lock_guard<std::mutex> lock(socketMutex_);
    if (fd_ >= 0) {
        return true;
    }

    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        Logger::error(std::string("socket failed: ") + std::strerror(errno));
        return false;
    }
    return true;
}

bool UdpSocket::bindAny(std::uint16_t port) {
    if (!open()) {
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        fd = fd_;
    }
    // A short receive timeout lets a blocking recvfrom re-check the caller's stop flag.
    timeval timeout{1, 0};
    (void)::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (::bind(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0) {
        Logger::error(std::string("bind failed: ") + std::strerror(errno));
        close();
        return false;
    }
    return true;
}

bool UdpSocket::sendTo(std::string_view payload, const sockaddr_in& peer) const {
    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        fd = fd_;
    }
    if (fd < 0) {
        Logger::error("sendto attempted on a closed socket");
        return false;
    }

    const ssize_t bytesSent = ::sendto(fd, payload.data(), payload.size(), 0,
                                       reinterpret_cast<const sockaddr*>(&peer), sizeof(peer));
    if (bytesSent < 0 || static_cast<std::size_t>(bytesSent) != payload.size()) {
        Logger::warn(std::string("sendto failed for ") + formatAddress(peer) + ": " + std::strerror(errno));
        return false;
    }
    return true;
}

void UdpSocket::setDatagramHandler(DatagramHandler handler) {
    handler_ = std::move(handler);
}

void UdpSocket::receiveLoop(const std::atomic_bool& running) const {
    std::array<char, kMaxDatagramBytes> buffer{};
    while (running.load()) {
        int fd = -1;
        {
            std::lock_guard<std::mutex> lock(socketMutex_);
            fd = fd_;
        }
        if (fd < 0) {
            return;
        }

        sockaddr_in peer{};
        socklen_t peerLength = sizeof(peer);
        const ssize_t bytesRead = ::recvfrom(fd, buffer.data(), buffer.size(), 0,
                                             reinterpret_cast<sockaddr*>(&peer), &peerLength);
        if (bytesRead < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            if (running.load()) {
                Logger::warn(std::string("recvfrom failed: ") + std::strerror(errno));
            }
            return;
        }

        if (handler_) {
            // Constructing a string copies the buffer before the next recvfrom reuse.
            handler_(Datagram{std::string(buffer.data(), static_cast<std::size_t>(bytesRead)), peer});
        }
    }
}

void UdpSocket::close() {
    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        fd = fd_;
        fd_ = -1;
    }
    if (fd >= 0) {
        ::close(fd);
    }
}

bool parseIpv4Address(std::string_view ip, std::uint16_t port, sockaddr_in& address) {
    address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    const std::string copy(ip);
    return ::inet_pton(AF_INET, copy.c_str(), &address.sin_addr) == 1;
}

std::string formatAddress(const sockaddr_in& address) {
    std::array<char, INET_ADDRSTRLEN> buffer{};
    const char* text = ::inet_ntop(AF_INET, &address.sin_addr, buffer.data(), buffer.size());
    std::ostringstream output;
    output << (text == nullptr ? "<invalid-address>" : text) << ':' << ntohs(address.sin_port);
    return output.str();
}

}  // namespace chat
