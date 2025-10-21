// 监听套接字模块
#pragma once
#include "Socket.hpp"
#include "EpollThread.hpp"

class Acceptor
{
using EpollThreadPtr = std::shared_ptr<EpollThread>;
private:
    Socket _sock;
    int _thread_num;
    int _next_thread;
    std::vector<EpollThreadPtr> _v;

    void Dispatcher(int fd)
    {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
        _v[_next_thread]->EpollCtl(EPOLL_CTL_ADD, fd, &event);
        _next_thread = (_next_thread + 1) % _thread_num;
    }
public:
    Acceptor(int thread_num)
    : _sock(false), _thread_num(thread_num), _next_thread(0)
    {
        for(int i = 0; i < _thread_num; i++)
        {
            EpollThreadPtr ptr = std::make_shared<EpollThread>();
            _v.push_back(ptr);
        }
        int optval = 1;
        setsockopt(_sock.GetFd(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        _sock.Bind(1025);
        _sock.Listen();
        while(true)
        {
            int newfd = _sock.Accept();
            if(newfd == -1)
                continue;
            Dispatcher(newfd);
        }
    }
};