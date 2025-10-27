#pragma once
#include "../Log/Log.hpp"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>

class RequestUtil
{
public:
    static std::string ParseForMethod(std::string &request)
    {
        std::string method;
        for (int i = 0; i < request.size(); i++)
        {
            if (request[i] == ' ')
            {
                request.erase(0, i + 1);
                return method;
            }
            method += request[i];
        }
        LOG(FATAL, "解析请求方法出错, 客户端构建的请求有问题:%s", request);
        exit(EXIT_FAILURE);
    }
    static std::string ParseForFilename(std::string &request)
    {
        std::string filename = "../UsersFile/";
        for (int i = 0; i < request.size(); i++)
        {
            if (request[i] == ' ')
            {
                request.erase(0, i + 1);
                return filename;
            }
            filename += request[i];
        }
        LOG(FATAL, "解析请求路径出错, 客户端构建的请求有问题:%s", request);
        exit(EXIT_FAILURE);
    }
    static std::pair<int, int> ParseForFileRange(std::string &request)
    {
        std::pair<int, int> file_range;
        std::string first;
        for (int i = 0; i < request.size(); i++)
        {
            if (request[i] == '-')
            {
                file_range.first = std::stoi(first);
                request.erase(0, i + 1);
                int pos = request.find('*');
                if (pos == std::string::npos)
                {
                    LOG(FATAL, "请求中没有*结束符, 客户端构建请求有问题:%s", request);
                    exit(EXIT_FAILURE);
                }
                std::string second = request.substr(0, pos);
                file_range.second = std::stoi(second);
                return file_range;
            }
            first += request[i];
        }
        LOG(FATAL, "解析文件范围出错, 找不到-, 客户端请求:%s", request);
        exit(EXIT_FAILURE);
    }
};

class FileUtil
{
public:
    static void FileNameOk(const std::string &filename)
    {
        std::string dir;
        int pos = filename.find('/');
        while (pos != std::string::npos)
        {
            dir = filename.substr(0, pos);
            if (mkdir(dir.c_str(), 0777) == -1 && errno != EEXIST)
            {
                LOG(FATAL, "文件路径有问题, %s, filename:%s", strerror(errno), filename.c_str());
                exit(EXIT_FAILURE);
            }
            pos = filename.find('/', pos + 1);
        }

        int fd = creat(filename.c_str(), 0666);
        if (fd == -1)
        {
            LOG(FATAL, "creat失败, filename:%s, %s", filename.c_str(), strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (close(fd) == -1)
        {
            LOG(FATAL, "关闭文件失败, fd:%d", fd);
            exit(EXIT_FAILURE);
        }
    }
};
