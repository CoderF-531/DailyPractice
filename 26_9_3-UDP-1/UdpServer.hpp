#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "Log.hpp"

using namespace LogModule;


//网络通信
class UdpServer{
public:
   
    UdpServer(uint16_t port)
    :_sockfd(-1)
    ,_port(port)
    ,_is_runing(false)
    {}

    void Init()
    {
        //1、创建套接字
        _sockfd = socket(AF_INET,SOCK_DGRAM,0);
        if(_sockfd<0)
        {
           LOG(LogLevel::FATAL)<<"socket error";
           exit(1);
        }
        LOG(LogLevel::INFO)<<"socket create success "<<_sockfd;
        //2、绑定地址信息
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET; // ip协议簇
        addr.sin_port = htons(_port);//端口
        addr.sin_addr.s_addr = INADDR_ANY; //
        
        int n = bind(_sockfd,(struct sockaddr*)&addr,sizeof(addr));
        if(n<0)
        {
            LOG(LogLevel::ERROR) << "bind failed for " << ":" << _port
                                  << ", errno=" << errno << " (" << std::strerror(errno) << ")";
            close(_sockfd);
            _sockfd = -1;
            exit(2);
        }
        LOG(LogLevel::INFO) << "UDP server listening on " <<":" << _port;

    }


    void Start()
    {
        _is_runing = true;
        while(_is_runing)
        {
            //开始通信
            char buffer[1024];
            struct sockaddr_in peeraddr;
            socklen_t len = sizeof(peeraddr);
            //接受消息
            ssize_t n = recvfrom(_sockfd,buffer,sizeof(buffer)-1,0,(struct sockaddr*)&peeraddr,&len);
            if(n>0)
            {
               //从网络中拿到 端口号
               int peerport = ntohs(peeraddr.sin_port);
               std::string peerip = inet_ntoa(peeraddr.sin_addr);
                buffer[n] = 0;
                LOG(LogLevel::INFO)<<"recvfrom "<<peerip<<":"<<peerport<<"#"<<buffer;
                //发送消息
                std::string echo = "server echo#";
                echo += buffer;
                sendto(_sockfd,echo.c_str(),echo.size(),0,(struct sockaddr*)&peeraddr,len);
            }     
        }
    }
    ~UdpServer() {}
private:
    int _sockfd;
    //std::string _ip;
    uint16_t _port;
    bool _is_runing;
};
