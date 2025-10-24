#pragma once
#include "../Log/Log.hpp"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>

void FileNameOk(const std::string& filename)
{
    std::string dir;
    int pos = filename.find_last_of('/');
    if(pos == std::string::npos)
    {
        LOG(FATAL, "filename找不到 '/' , filename:%s , filename有问题", filename.c_str());
        exit(EXIT_FAILURE);
    }
    dir = filename.substr(0, pos);
    if(mkdir(dir.c_str(), 0777) == -1)
    {
        LOG(FATAL, "目录名有问题, 目录名:%s, %s", dir.c_str(), strerror(errno));
        exit(EXIT_FAILURE);
    }
    int fd = creat(filename.c_str(), 0666);
    if(fd == -1)
    {
        LOG(FATAL, "creat失败, filename:%s, %s", filename.c_str(), strerror(errno));
        exit(EXIT_FAILURE);
    }
    if(close(fd) == -1)
    {
        LOG(FATAL, "关闭文件失败, fd:%d", fd);
        exit(EXIT_FAILURE);
    }
}