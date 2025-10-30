// 日志系统
// 等级 时间 文件名+行号 日志消息
#pragma once
#include <string>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <cstring>

enum LogLevel
{
    INFO,
    DEBUG,
    WARN,
    ERROR,
    FATAL
};
class Logger
{
#define MsgSize 1000
private:
    std::string LevelToString(LogLevel level)
    {
        switch (level)
        {
        case INFO:
            return "INFO";
        case DEBUG:
            return "DEBUG";
        case WARN:
            return "WARN";
        case ERROR:
            return "ERROR";
        case FATAL:
            return "FATAL";
        default:
            std::cout << "输入的日志等级有问题, level" << level << std::endl;
            return "";
        }
    }

    void Log(LogLevel level, const std::string &filename, int line, const std::string &msg_format, va_list ap)
    {

        char msg[MsgSize] = {0};
        vsnprintf(msg, MsgSize, msg_format.c_str(), ap);

        char time_str[50] = {0};
        time_t t;
        time(&t);
        struct tm *broken_down_t = localtime(&t);
        strftime(time_str, 50, "%m-%d %T", broken_down_t);

        std::string final_msg = "[" + LevelToString(level) + "]";
        final_msg = final_msg + "[" + time_str + "]";
        final_msg = final_msg + "[" + filename + ":" + std::to_string(line) + "]";
        final_msg += msg;
        std::cout << final_msg << std::endl;
    }

public:
    void operator()(LogLevel level, const std::string &filename, int line, const std::string &msg_format, ...)
    {
        va_list ap;
        va_start(ap, msg_format);
        Log(level, filename, line, msg_format, ap);
        va_end(ap);
    }
};

Logger Log;
#define LOG(level, msg_format, ...) Log(level, __FILE_NAME__, __LINE__, msg_format, ##__VA_ARGS__)