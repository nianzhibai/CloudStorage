// 文件描述符监听事件模块
#pragma once
#include "../Log/Log.hpp"
#include "Buffer.hpp"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <fstream>

class EpollThread
{
    using BufferPtr = std::shared_ptr<Buffer>;

private:
    std::thread _t;
    int _efd;
    int _eventfd;
    std::unordered_map<int, BufferPtr> _hash;

    std::mutex _mtx;
    bool _epoll_ctl;
    std::condition_variable cond;

    void ProcessData(const BufferPtr &buffer)
    {
        // if (buffer->_has_a_request == false)
        // {
        //     std::string req;
        //     if (socket_buffer->_inbuffer.ReadRequestFromBuffer(req) == false)
        //     {
        //         LOG(FATAL, "套接字:%d有读事件就绪, 但是无法从发来的数据解析出一个请求", socket_buffer->_sockfd);
        //         exit(EXIT_FAILURE);
        //     }
        //     socket_buffer->SetRawRequest(req);
        //     socket_buffer->_has_a_request = true;
        // }

        // std::ofstream ofs(socket_buffer->_request_filename);
        // while(socket_buffer->need_to_recv_size != socket_buffer->recved_size)
        // {

        // }
        
        // 处理第一次任务（设置标识为）
        // 换新等待条件变量的线程
    }

    void ReadEventFunc(const BufferPtr &buffer)
    {
        {
            std::lock_guard<std::mutex> guard(buffer->_mtx);
            buffer->RecvInBuffer();
        }
        ProcessData(buffer);
    }

    void WriteEventFunc(const BufferPtr &buffer)
    {
    }

    void CloseEventFunc(const BufferPtr &buffer)
    {
        // if (socket_buffer->_outbuffer.ReadAbleSize() != 0)
        // {
        //     struct epoll_event event;
        //     event.data.fd = socket_buffer->_sockfd;
        //     event.events = EPOLLIN | EPOLLOUT | EPOLLHUP;
        //     EpollCtl(EPOLL_CTL_ADD, socket_buffer->_sockfd, &event);
        //     return;
        // }
        // EpollCtl(EPOLL_CTL_DEL, socket_buffer->_sockfd, nullptr);
    }
    void ErrorEventFunc(const BufferPtr &buffer)
    {
        // LOG(INFO, "套接字:%d出现问题", socket_buffer->_sockfd);
        // EpollCtl(EPOLL_CTL_DEL, socket_buffer->_sockfd, nullptr);
    }

    void ReadyEventsHandle(int n, struct epoll_event *ready_events)
    {
        for (int i = 0; i < n; i++)
        {
            int fd = ready_events[i].data.fd;
            uint32_t events = ready_events[i].events;
            if (fd == _eventfd)
            {
                char buf[8] = {0};
                read(_eventfd, &buf, 8);
                continue;
            }

            if (events & EPOLLERR)
            {
                ErrorEventFunc(_hash[fd]);
                continue;
            }
            if (events & EPOLLRDHUP)
            {
                CloseEventFunc(_hash[fd]);
                continue;
            }
            if (events & EPOLLIN)
            {
                ReadEventFunc(_hash[fd]);
            }
            if (events & EPOLLOUT)
            {
                WriteEventFunc(_hash[fd]);
            }
        }
    }
    void ThreadFunc()
    {
        struct epoll_event ready_events[1024];
        while (true)
        {
            memset(ready_events, 0, sizeof(epoll_event) * 1024);
            std::unique_lock<std::mutex> lock(_mtx);
            cond.wait(lock, [this]() -> bool
                      { return !_epoll_ctl; });
            lock.unlock();

            // LOG(INFO, "我是efd:%d, 我要开始epoll_wait了", _efd);
            int ret = epoll_wait(_efd, ready_events, 1024, -1);
            // LOG(INFO, "我是efd:%d, epoll_wait返回, 有:%d个描述符上事件就绪了", _efd, ret);
            // for(int i = 0; i < ret; i++)
            // {
            //     std::cout << ready_events[i].data.fd << " ";
            // }
            // std::cout << std::endl;

            if (ret == -1)
            {
                LOG(FATAL, "epoll_wait fail, %s, efd:%d", strerror(errno), _efd);
                exit(EXIT_FAILURE);
            }
            ReadyEventsHandle(ret, ready_events);
        }
    }

public:
    EpollThread()
        : _epoll_ctl(true)
    {
        _efd = epoll_create(1);
        if (_efd == -1)
        {
            LOG(FATAL, "epoll_create fail, %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        _eventfd = eventfd(0, EFD_NONBLOCK);
        if (_eventfd == -1)
        {
            LOG(FATAL, "eventfd fail, %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = _eventfd;
        if (epoll_ctl(_efd, EPOLL_CTL_ADD, _eventfd, &event) == -1)
        {
            LOG(FATAL, "向epoll中添加eventfd失败, %s, _efd:%d", strerror(errno), _efd);
            exit(EXIT_FAILURE);
        }

        _t = std::thread(&EpollThread::ThreadFunc, this);
    }

    void EpollCtl(int op, int fd, struct epoll_event *event)
    {
        _epoll_ctl = true;
        uint64_t value = 1;
        write(_eventfd, &value, 8);

        if (epoll_ctl(_efd, op, fd, event) == -1)
        {
            if (op == EPOLL_CTL_ADD)
            {
                LOG(FATAL, "epoll_ctl fail, %s. op:%s. efd:%d. fd:%d", strerror(errno), "EPOLL_CTL_ADD", _efd, fd);
                exit(EXIT_FAILURE);
            }
            if (op == EPOLL_CTL_MOD)
            {
                LOG(FATAL, "epoll_ctl fail, %s. op:%s. efd:%d. fd:%d", strerror(errno), "EPOLL_CTL_MOD", _efd, fd);
                exit(EXIT_FAILURE);
            }
            if (op == EPOLL_CTL_DEL)
            {
                LOG(FATAL, "epoll_ctl fail, %s. op:%s. efd:%d. fd:%d", strerror(errno), "EPOLL_CTL_DEL", _efd, fd);
                exit(EXIT_FAILURE);
            }
        }

        if (op == EPOLL_CTL_ADD)
        {
            BufferPtr ptr = std::make_shared<Buffer>(fd);
            _hash[fd] = ptr;
            LOG(INFO, "套接字:%d添加到了efd:%d中", fd, _efd);
        }
        else if (op == EPOLL_CTL_DEL)
        {
            _hash.erase(fd);
            LOG(INFO, "套接字:%d被移除", fd);
        }
        else
        {
        }

        _epoll_ctl = false;
        cond.notify_one();
    }

    ~EpollThread()
    {
        _t.join();
    }
};