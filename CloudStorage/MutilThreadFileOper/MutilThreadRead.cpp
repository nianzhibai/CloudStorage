#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>

std::mutex mtx;
std::condition_variable cond;
bool main_thread_ok = false;
std::string total = "";

void ThreadFunc(int filesize, int pos)
{
    std::ifstream ifs("output.mp4", std::ios_base::binary);
    ifs.seekg(pos);
    int remain_size = filesize - pos;
    std::string s2(remain_size, 0);
    ifs.readsome(&s2[0], remain_size);
    if (ifs.good() == false)
    {
        std::cout << "文件操作有问题" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::unique_lock<std::mutex> lock(mtx);
    cond.wait(lock, []() {return main_thread_ok;});
    total += s2;
}

int main()
{
    std::ifstream ifs("output.mp4", std::ios_base::binary | std::ios_base::ate);
    int filesize = ifs.tellg();
    std::cout << "filesize" << filesize << std::endl;
    int pos = filesize / 2;

    std::thread t(&ThreadFunc, filesize, pos);

    std::string s1(pos, 0);
    ifs.readsome(&s1[0], pos);
    if (ifs.good() == false)
    {
        std::cout << "文件操作有问题" << std::endl;
        exit(EXIT_FAILURE);
    }
    total += s1;
    main_thread_ok = true;
    cond.notify_one();

    t.join();

    std::cout << "total" << total.size() << std::endl;
    return 0;
}