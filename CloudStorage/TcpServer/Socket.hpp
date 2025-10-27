// 网络套接字模块
#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../Log/Log.hpp"

class Socket
{
private:
    int _sockfd;
public:
    // 创建套接字
    Socket(bool NoNBlock = true)
    {
        if(NoNBlock)
            _sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        else
            _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(_sockfd == -1)
        {
            LOG(FATAL, "套接字创建失败，%s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    Socket(int fd) { _sockfd = fd; }
    // 绑定IP
    void Bind(u_int16_t port)
    {
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = INADDR_ANY;

        if(bind(_sockfd, (sockaddr*)&server, sizeof(server)) == -1)
        {
            LOG(FATAL, "套接字: %d, 绑定地址失败, %s", _sockfd, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    // 监听套接字
    void Listen()
    {
        if(listen(_sockfd, 10) == -1)
        {
            LOG(FATAL, "监听套接字失败, %s, 套接字:%d", strerror(errno), _sockfd);
            exit(EXIT_FAILURE);
        }
    }
    // 接受连接
    int Accept()
    {
        int ret = accept4(_sockfd, nullptr, nullptr, SOCK_NONBLOCK);
        if(ret == -1)
        {
            LOG(INFO, "套接字:%d, 接受连接失败, %s", _sockfd, strerror(errno));
            return -1;
        }
        return ret;
    }
    // 发起连接
    void Connect(const std::string& ip, u_int16_t port)
    {
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        inet_aton(ip.c_str(), &(server.sin_addr));
        if(connect(_sockfd, (sockaddr*)&server, sizeof(server)) == -1)
        {
            if(errno == EINPROGRESS)
                return;
            else
            {
                LOG(FATAL, "连接失败, %s", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    }

    int GetFd() { return _sockfd; }

    ~Socket()
    {
        close(_sockfd);
    }
};