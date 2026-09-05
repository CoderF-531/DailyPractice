#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <string_view>

namespace chat {

struct Datagram {
    std::string payload;
    sockaddr_in peer{};
};

// The callback receives an owning Datagram. Its payload remains valid after the
// next recvfrom call, so it is safe for the callback to submit it to a thread pool.
using DatagramHandler = std::function<void(Datagram)>;

class UdpSocket {
public:
    UdpSocket() = default;
    ~UdpSocket();

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    bool bindAny(std::uint16_t port);
    bool sendTo(std::string_view payload, const sockaddr_in& peer) const;
    void setDatagramHandler(DatagramHandler handler);

    // Blocks in recvfrom while running is true. Closing the socket wakes a blocked receiver.
    // The handler runs on this caller's thread and must return quickly.
    void receiveLoop(const std::atomic_bool& running) const;
    void close();

private:
    bool open();

    int fd_ = -1;
    mutable std::mutex socketMutex_;
    DatagramHandler handler_;
};

bool parseIpv4Address(std::string_view ip, std::uint16_t port, sockaddr_in& address);
std::string formatAddress(const sockaddr_in& address);

}  // namespace chat
