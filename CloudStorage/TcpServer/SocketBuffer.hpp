#pragma once
#include "../Log/Log.hpp"
#include <vector>
#include <algorithm>
#include <unistd.h>

class Buffer
{
private:
    std::vector<char> _buffer;
    uint _read_idx;
    uint _write_idx;

    void Expand(int size)
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
            _buffer.resize(_buffer.size() * 2 + size);
            return;
        }
    }

public:
    Buffer()
        : _buffer(4096, 0), _read_idx(0), _write_idx(0) {}

    int WriteAbleSize() { return _buffer.size() - _write_idx + _read_idx; }
    int ReadAbleSize() { return _write_idx - _read_idx; }

    char *ReadPos() { return &_buffer[_read_idx]; }
    char *WritePos() { return &_buffer[_write_idx]; }

    void WriteInBuffer(uint size, const void *data)
    {
        Expand(size);
        std::copy((const char *)data, (const char *)data + size, WritePos());
        _write_idx += size;
    }
    const std::string ReadFromBuffer()
    {
        std::string str(ReadAbleSize(), 0);
        std::copy(ReadPos(), WritePos(), &str[0]);
        _read_idx = _write_idx = 0;
        return str;
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
};

class SocketBuffer
{
public:
    Buffer _inbuffer;
    Buffer _outbuffer;
    int _sockfd;

    SocketBuffer(int sockfd)
        : _sockfd(sockfd) {}
    ~SocketBuffer() 
    {
        int ret = close(_sockfd); 
        if(ret == -1)
            LOG(INFO, "close sockfd:%d fail, %s", _sockfd, strerror(errno));
    }
};