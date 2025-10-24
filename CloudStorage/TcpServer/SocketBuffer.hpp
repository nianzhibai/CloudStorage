#pragma once
#include "../Log/Log.hpp"
#include "RequestUtil.hpp"
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
};

class SocketBuffer
{
public:
    Buffer _inbuffer;
    Buffer _outbuffer;
    int _sockfd;

    int _has_a_request;
    std::string _raw_request;
    std::string _request_method;
    std::string _request_filename;
    std::pair<int, int> _request_file_range;
    int need_to_recv_size;
    int recved_size;

    SocketBuffer(int sockfd)
        : _sockfd(sockfd), _has_a_request(false) {}

    void SetRawRequest(std::string request)
    {
        _raw_request = request;
        _request_method = RequestUtil::ParseForMethod(request);
        _request_filename = RequestUtil::ParseForFilename(request);
        _request_file_range = RequestUtil::ParseForFileRange(request);
        need_to_recv_size = _request_file_range.second - _request_file_range.first;
        recved_size = 0;
    }

    ~SocketBuffer()
    {
        int ret = close(_sockfd);
        if (ret == -1)
            LOG(INFO, "close sockfd:%d fail, %s", _sockfd, strerror(errno));
    }
};