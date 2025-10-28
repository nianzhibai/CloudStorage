#pragma once
#include "../Log/Log.hpp"
#include "Util.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <mutex>
#include <condition_variable>

class Buffer
{
private:
    std::vector<char> _buffer;
    uint _read_idx;
    uint _write_idx;
    int _sockfd;

    void ExpandCapacity(int size)
    {
        if (_buffer.size() - _write_idx >= size)
            return;
        else if (WriteAbleSize() >= size)
        {
            std::copy(ReadPos(), WritePos(), _buffer.begin());
            _write_idx = _write_idx - _read_idx;
            _read_idx = 0;
            return;
        }
        else
        {
            _buffer.reserve(_buffer.size() * 2 + size);
            return;
        }
    }
    
public:
    std::mutex _mtx;
    std::condition_variable _cond;
    
    Buffer(int sockfd)
        : _buffer(1024 * 1024 * 10, 0), _read_idx(0), _write_idx(0), _sockfd(sockfd) {}

    int WriteAbleSize() { return _buffer.capacity() - _write_idx + _read_idx; }
    int ReadAbleSize() { return _write_idx - _read_idx; }

    char *ReadPos() { return &_buffer[_read_idx]; }
    char *WritePos() { return &_buffer[_write_idx]; }

    void RecvInBuffer()
    {
        while (true)
        {
            if (ReadAbleSize() < 4096)
            {
                ExpandCapacity(4096);
            }
            int ret = recv(_sockfd, &_buffer[_write_idx], 1024, 0);
            if (ret < 0)
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                    return;
                if (errno == EINTR)
                    continue;
                else
                {
                    LOG(FATAL, "套接字出现问题, %s", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
            else if (ret == 0)
            {
                LOG(FATAL, "客户端要关闭连接, 这是不正常的, 宝贝代码写错了吧");
                exit(EXIT_FAILURE);
            }
            else
            {
                _write_idx += ret;
            }
        }
    }
    void OutWardData(std::vector<char> &v)
    {
        v.resize(ReadAbleSize());
        std::copy(ReadPos(), WritePos(), v.begin());
        _read_idx = _write_idx = 0;
    }

    void WriteInBuffer(uint size, const void *data)
    {
        ExpandCapacity(size);
        std::copy((const char *)data, (const char *)data + size, WritePos());
        _write_idx += size;
    }
    const std::string ReadAllFromBuffer()
    {
        std::string str(ReadAbleSize(), 0);
        std::copy(ReadPos(), WritePos(), &str[0]);
        _read_idx = _write_idx = 0;
        return str;
    }
    bool ReadRequestFromBuffer(std::string &request)
    {
        for (int i = _read_idx; i < ReadAbleSize(); i++)
        {
            if (_buffer[i] == '*')
            {
                request += _buffer[i];
                _read_idx += (i - _read_idx + 1);
                return true;
            }
            request += _buffer[i];
        }
        request = "";
        return false;
    }
    void MoveReadIdx(int size)
    {
        if (size > ReadAbleSize())
        {
            LOG(INFO, "不能移动ReadIdx Size大小, 因为可读数据没有那么多");
            return;
        }
        _read_idx += size;
    }
    void Reset() { _read_idx = _write_idx = 0; }
    int Capacity() { return _buffer.capacity(); }
};