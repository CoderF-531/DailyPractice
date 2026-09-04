#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>

typedef struct sockaddr_in sockaddr_in;
class InetAddr
{
public:
    InetAddr(struct sockaddr_in& addr) : m_addr(addr)
    {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
        m_ip = std::string(ip);
        m_port = ntohs(addr.sin_port);
    }
    uint16_t Port()const { return m_port; }
    std::string IP()const { return m_ip; }
    ~InetAddr() = default;

private:
    sockaddr_in m_addr;
    std::string m_ip;
    uint16_t m_port;
};