#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "Log.hpp"


using namespace LogModule;

int main(int argc,char* argv[])
{
      if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " server_ip server_port" << std::endl;
        return 1;
    }

    //接受 ip 和端口
    std::string client_ip = argv[1];
    uint16_t client_port = atoi(argv[2]);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        LOG(LogLevel::FATAL) << "socket error";
        exit(1);
    }
    //设置服务器地址信息
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET; //ip协议簇
    server_addr.sin_addr.s_addr = inet_addr(client_ip.c_str()); //ip
    server_addr.sin_port = htons(client_port); //端口

      // 2. 本地的ip和端口是什么？要不要和上面的“文件”关联呢？
    // 问题：client要不要bind？需要bind.
    //       client要不要显式的bind?不要！！首次发送消息，OS会自动给client进行bind，OS知道IP，端口号采用随机端口号的方式
    //   为什么？一个端口号，只能被一个进程bind，为了避免client端口冲突
    //   client端的端口号是几，不重要，只要是唯一的就行！
    // 填写服务器信息
    while(true)
    {
      std::string input;
        std::cout << "Enter message to send (or 'exit' to quit): ";
        std::getline(std::cin,input);
        int n = sendto(sockfd,input.c_str(),input.size(),0,(struct sockaddr*)&server_addr,sizeof(server_addr));
        (void)n;

        //读取/收信息
        char buffer[1024];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        ssize_t recv_len = recvfrom(sockfd,buffer,sizeof(buffer)-1,0,(struct sockaddr*)&from_addr,&from_len);
        if(recv_len>0)
        {
            buffer[recv_len] = '\0'; // Null-terminate the received data
            std::cout << "Received from server: " << buffer << std::endl;
        }
        else
        {
            LOG(LogLevel::ERROR) << "recvfrom error";
        }
    }


    return 0;
}