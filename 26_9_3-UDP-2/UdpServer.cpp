#include "UdpServer.hpp"
#include "Log.hpp"
#include <memory>
#include <limits>
#include "Dict.hpp"
using namespace LogModule;


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

    std::string EchoTask(const std::string &request)
    {
        // 这里可以根据请求内容进行处理，返回响应
        std::string hello = "hello " + request;
        return hello;
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
    
    Dict dict;
    dict.Load(); //加载字典文件

    std::unique_ptr<UdpServer> udpserver = std::make_unique<UdpServer>(port,[&dict](const std::string &word, InetAddr&cli) -> std::string {
              return dict.Translate(word, cli);
    });
    udpserver->Init();
    udpserver->Start();
    return 0;
}