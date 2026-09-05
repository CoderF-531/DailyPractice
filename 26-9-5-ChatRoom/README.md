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
