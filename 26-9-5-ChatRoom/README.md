# 终端 UDP 聊天室

Linux 下的 C++17 学习项目，包含 UDP Socket、线程池、日志和客户端/服务端通信。

## 构建

```bash
make
make test
```

## 运行

```bash
# 服务端绑定 0.0.0.0，默认端口 9000
./bin/chat_server
./bin/chat_server 9001

# 客户端：<服务端 IP> <昵称> [端口]
./bin/chat_client 127.0.0.1 alice
./bin/chat_client 192.168.1.10 bob 9001
```

服务端和客户端在同一台机器时使用 `127.0.0.1`。输入 `/quit` 可退出客户端。

## 代码学习

第一次阅读代码，建议从 [代码学习指南](docs/代码学习指南.md) 开始，按 Makefile、协议、Socket、回调、线程池、服务端、客户端的顺序逐层理解。
