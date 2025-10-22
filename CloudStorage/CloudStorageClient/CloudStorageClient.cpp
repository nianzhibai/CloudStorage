#include "../Log/Log.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <fstream>

std::vector<int> sockfd_v;
std::vector<std::thread> thread_v;
std::mutex mtx;
std::condition_variable cond;
bool has_task = false;
int task_opt = -1;
int file_mid = 0, file_end = 0;

void ThreadFunc1()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, []()
                  { return has_task; });
        switch (task_opt)
        {
        case 1:
        {
        }
        }
    }
}
void ThreadFunc2();
void ThreadFunc3();

void Menu()
{
    std::cout << "**************************************" << std::endl;
    std::cout << "Please select the corresponding number" << std::endl;
    std::cout << "*****0.Exit                      *****" << std::endl;
    std::cout << "*****1.UploadFile  2.DownloadFile*****" << std::endl;
    std::cout << "*****3.DeleteFile  4.ShowCurFiles*****" << std::endl;
    std::cout << "**************************************" << std::endl;
}

void ExitFunc()
{
    for (auto e : sockfd_v)
    {
        if (close(e) == -1)
        {
            LOG(FATAL, "关闭套接字失败:%d, %s", e, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

bool HasFile(const std::string &filename)
{
    struct stat statbuf;
    if (stat(filename.c_str(), &statbuf) == -1)
    {
        if (errno == ENOENT)
        {
            std::cout << "文件名错误, 找不到改文件" << std::endl;
            return false;
        }
        else
        {
            std::cout << "文件名有问题, 其他问题" << std::endl;
            return false;
        }
    }
    return true;
}
int FileSize(const std::string &filename)
{
    struct stat statbuf;
    if (stat(filename.c_str(), &statbuf) == -1)
    {
        LOG(FATAL, "stat获取文件:%s属性失败, %s", filename.c_str(), strerror(errno));
        exit(EXIT_FAILURE);
    }
    int filesize = statbuf.st_size;
    return filesize;
}

void UploadFunc(const std::string &filename)
{
    int filesize = FileSize(filename);
    if (filesize > 1024 * 1024 * 100)
    {
        has_task = true;
        task_opt = 1;
        file_mid = filesize / 2;
        file_end = filesize;
        cond.notify_all();
    }
    else
    {
        std::ifstream ifs(filename.c_str(), std::ios_base::binary);
        // read一些发一些
        ifs.read();
    }
}

int main()
{
    for (int i = 0; i < 3; i++)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
            LOG(FATAL, "创建套接字失败, %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        inet_aton("127.0.0.1", &(server_addr.sin_addr));
        server_addr.sin_port = htons(1025);
        if (connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
            LOG(FATAL, "套接字:%d连接服务器失败, %s", sockfd, strerror(errno));
            exit(EXIT_FAILURE);
        }
        sockfd_v.push_back(sockfd);
    }
    thread_v.emplace_back(ThreadFunc1);
    thread_v.emplace_back(ThreadFunc2);
    thread_v.emplace_back(ThreadFunc3);

    while (true)
    {
        int input = 0;
        Menu();
        std::cin >> input;
        while (!(input >= 0 && input <= 4))
        {
            std::cout << "请从新输入正确的数字:";
            std::cin >> input;
        }

        switch (input)
        {
        case 0:
        {
            ExitFunc();
            std::cout << "退出成功" << std::endl;
            exit(EXIT_SUCCESS);
        }
        case 1:
        {
            std::string filename;
            std::cout << "请输入你要上传文件的文件名:";
            std::cin >> filename;
            while (HasFile(filename) == false)
            {
                std::cout << "请重新输入文件名:";
                std::cin >> filename;
            }
            UploadFunc(filename);
            // 等异步线程完成工作后才能别的操作
        }
        }
    }
}