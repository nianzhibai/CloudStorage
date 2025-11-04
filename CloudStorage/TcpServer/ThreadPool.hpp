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
    std::string method;
    int begin;
    int end;
    std::vector<char> data;
    BufferPtr ptr;

    Task(const std::string &filename_, const std::string &method_, int begin_, int end_, BufferPtr ptr_)
        : filename(filename_), method(method_), begin(begin_), end(end_), ptr(ptr_) {}
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
            if (task.method == "Upload")
            {
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
            else if (task.method == "Download")
            {
                std::ifstream ifs(task.filename, std::ios_base::binary);
                int need_to_read = task.end - task.begin;
                LOG(INFO, "线程%p需要给客户端发送文件%s中%d到%d的数据", std::this_thread::get_id(), task.filename.c_str(), task.begin, task.end);
                ifs.seekg(task.begin);
                if (ifs.rdstate() == std::ios_base::badbit)
                {
                    LOG(FATAL, "线程%p移动文件指针出错", std::this_thread::get_id());
                    exit(EXIT_FAILURE);
                }
                while (need_to_read != 0)
                {
                    const int buf_size = need_to_read >= 4096 ? 4096 : need_to_read;
                    char buf[buf_size] = {0};
                    ifs.read(buf, buf_size);
                    int num = ifs.gcount();
                    if (num != buf_size)
                    {
                        LOG(INFO, "线程%p没有读取到应该数量的数据", std::this_thread::get_id());
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        int ret = send(task.ptr->_sockfd, buf, buf_size, 0);
                        if (ret < 0)
                        {
                            if (errno == EWOULDBLOCK || errno == EAGAIN)
                            {
                                ifs.seekg(task.end - need_to_read);
                                continue;
                            }
                            else
                            {
                                LOG(FATAL, "套接字:%d有问题, %s", task.ptr->_sockfd, strerror(errno));
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (ret > 0)
                        {
                            if (ret == buf_size)
                            {
                                need_to_read -= buf_size;
                                continue;
                            }
                            else
                            {
                                need_to_read -= ret;
                                ifs.seekg(task.end - need_to_read);
                                continue;
                            }
                        }
                        else
                        {
                            ifs.seekg(task.end - need_to_read);
                            continue;
                        }
                    }
                }
                ifs.close();
                task.ptr->_has_a_request = false;
                task.ptr->Clear();
                LOG(INFO, "线程%p给客户端发送文件%s中%d到%d的数据Success", std::this_thread::get_id(), task.filename.c_str(), task.begin, task.end);
            }
            else
            {
            }
        }
    }

public:
    ThreadPool()
    {
        for (int i = 0; i < 5; i++)
            _threads.emplace_back(&ThreadPool::ThreadFunc, this);
    }

    void AddTask(const std::string &filename, const std::string &method, int begin, int end, std::shared_ptr<Buffer> ptr)
    {
        std::lock_guard<std::mutex> guard(_mtx);
        _tasks.emplace_back(filename, method, begin, end, ptr);
        _cond.notify_one();
    }
};