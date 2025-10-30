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
    std::condition_variable _cond;

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
            std::ofstream ofs(task.filename, std::ios_base::binary | std::ios_base::in | std::ios_base::out);
            if (ofs.rdstate() != ofs.goodbit)
            {
                LOG(INFO, "文件没有打开成功, 文件:%s", task.filename.c_str());
                exit(EXIT_FAILURE);
            }
            ofs.seekp(task.begin);
            if (ofs.rdstate() != ofs.goodbit)
            {
                LOG(INFO, "seekp操作失败, 文件:%s", task.filename.c_str());
                exit(EXIT_FAILURE);
            }
            // LOG(INFO, "线程%p打开文件起始位置是%d", std::this_thread::get_id(), ofs.tellp());

            int tmp = task.begin;
            while (tmp != task.end)
            {
                std::unique_lock<std::mutex> lock2(task.ptr->_mtx);
                task.ptr->_cond.wait(lock2, [&task]()
                                     { return task.ptr->ReadAbleSize(); });
                task.ptr->OutWardData(task.data);
                lock2.unlock();

                // LOG(INFO, "线程%p准备写入数据, 写入位置%d", std::this_thread::get_id(), ofs.tellp());
                ofs.write(&task.data[0], task.data.size());
                if (ofs.rdstate() != ofs.goodbit)
                {
                    LOG(INFO, "seekp操作失败, 文件:%s", task.filename.c_str());
                    exit(EXIT_FAILURE);
                }
                tmp += task.data.size();
            }
            ofs.close();
            task.ptr->_has_a_request = false;
            task.ptr->Clear();
            LOG(INFO, "%p线程落地%s文件的%d到%d的内容Success", std::this_thread::get_id(), task.filename.c_str(), task.begin, task.end, ofs.tellp());
        }
    }

public:
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