#pragma once
#include "../Log/Log.hpp"
#include "Buffer.hpp"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <thread>

struct Task
{
    using BufferPtr = std::shared_ptr<Buffer>;

    std::string filename;
    int begin;
    int end;
    std::vector<char> data;
    BufferPtr ptr;

    Task(const std::string &filename_, int begin_, int end_, BufferPtr ptr_)
        : filename(filename_), begin(begin_), end(end_), ptr(ptr_) {}
};
class ThreadPool
{
private:
    std::vector<Task> _tasks;
    std::vector<std::thread> _threads;
    std::mutex _mtx;

    void ThreadFunc()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock1(_mtx);
            _cond.wait(lock1, [this]()
                       { return !_tasks.empty(); });
            Task task = std::move(_tasks.back());
            _tasks.pop_back();
            LOG(INFO, "%p线程拿到套接字%d上的任务, 落地%s文件的%d到%d的内容", std::this_thread::get_id(), task.ptr->_sockfd, task.filename.c_str(), task.begin, task.end);
            lock1.unlock();

            std::ofstream ofs(task.filename, std::ios_base::binary);
            if (ofs.rdstate() != ofs.goodbit)
            {
                LOG(INFO, "文件没有打开成功, 文件:%s", task.filename.c_str());
                exit(EXIT_FAILURE);
            }
            while (task.begin != task.end)
            {
                std::unique_lock<std::mutex> lock2(task.ptr->_mtx);
                task.ptr->_cond.wait(lock2, [&task]()
                                     { return task.ptr->ReadAbleSize(); });
                task.ptr->OutWardData(task.data);
                lock2.unlock();

                ofs.seekp(task.begin);
                if (ofs.rdstate() != ofs.goodbit)
                {
                    LOG(INFO, "seekp操作失败, 文件:%s", task.filename.c_str());
                    exit(EXIT_FAILURE);
                }
                ofs.write(&task.data[0], task.data.size());
                if (ofs.rdstate() != ofs.goodbit)
                {
                    LOG(INFO, "seekp操作失败, 文件:%s", task.filename.c_str());
                    exit(EXIT_FAILURE);
                }
                task.begin += task.data.size();
            }
            ofs.close();
            task.ptr->_has_a_request = false;
            task.ptr->Clear();
            LOG(INFO, "%p线程落地%s文件的%d到%d的内容Success", std::this_thread::get_id(), task.filename.c_str(), task.begin, task.end);
        }
    }

public:
    std::condition_variable _cond;

    ThreadPool()
    {
        for (int i = 0; i < 5; i++)
            _threads.emplace_back(&ThreadPool::ThreadFunc, this);
    }

    void AddTask(const std::string &filename, int begin, int end, std::shared_ptr<Buffer> ptr)
    {
        std::lock_guard<std::mutex> guard(_mtx);
        _tasks.emplace_back(filename, begin, end, ptr);
        _cond.notify_one();
    }
};