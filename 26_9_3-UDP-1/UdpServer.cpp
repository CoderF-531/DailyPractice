#include "UdpServer.hpp"
#include "Log.hpp"
#include <memory>
#include <limits>

using namespace LogModule;

namespace
{
    bool ParsePort(const char *text, uint16_t &port)
    {
        char *end = nullptr;
        unsigned long value = std::strtoul(text, &end, 10);
        if (text[0] == '\0' || *end != '\0' || value == 0 ||
            value > std::numeric_limits<uint16_t>::max())
        {
            return false;
        }
        port = static_cast<uint16_t>(value);
        return true;
    }
}

int main(int argc, char* argv[])
{
    Enable_Console_Log_Strategy(); //屏幕日志

    //std::string ip = "0.0.0.0";
    uint16_t port = 8080;
    if (argc == 2)
    {
        if (!ParsePort(argv[1], port))
        {
            LOG(LogLevel::ERROR) << "invalid port: " << argv[1];
            return 1;
        }
    }
 

    std::unique_ptr<UdpServer> udpserver = std::make_unique<UdpServer>(port);
    udpserver->Init();
    udpserver->Start();
    return 0;
}
