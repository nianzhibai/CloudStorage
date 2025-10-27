#pragma once
#include "../Log/Log.hpp"
#include "Buffer.hpp"
#include <string>
#include <vector>
#include <memory>

struct Task
{
    using BufferPtr = std::unique_ptr<Buffer>;

    std::string filename;
    int begin;
    int end;
    std::vector<char> data;
    BufferPtr ptr;
};
class ThreadPool
{

};