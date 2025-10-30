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

std::string user_base_dir = "Su/";
std::vector<int> sockfd_v;
std::vector<std::thread> thread_v;
std::mutex mtx1;
std::mutex mtx2;
std::condition_variable cond_to_other_thread;
std::condition_variable cond_to_main_thread;
bool has_task_first_thread = false;
bool has_task_second_thread = false;
bool has_task_third_thread = false;
int task_finish = 3;
int task_opt = -1;
int file_mid1 = 0, file_mid2 = 0, file_end = 0;
std::string filename;

void ThreadFunc1()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx1);
        cond_to_other_thread.wait(lock, []()
                                  { return has_task_first_thread; });
        lock.unlock();
        LOG(INFO, "我是线程1, 现在有任务了");
        switch (task_opt)
        {
        case 1:
        {
            std::string upload_request = "Upload";
            upload_request = upload_request + " " + user_base_dir + filename + " " + "0" + "-" + std::to_string(file_mid1) + "*.*";
            int ret = send(sockfd_v[0], upload_request.c_str(), upload_request.size(), 0);
            if (ret < 0)
            {
                LOG(FATAL, "套接字:%d出现问题%s, 发送请求:%s失败", sockfd_v[0], strerror(errno), upload_request.c_str());
                exit(EXIT_FAILURE);
            }
            std::ifstream ifs(filename.c_str() , std::ios_base::binary);
            int need_to_read = file_mid1;
            LOG(INFO, "线程1需要给服务器发送文件%s中%d到%d的数据", filename.c_str(), 0, file_mid1);
            while (need_to_read != 0)
            {
                const int buf_size = need_to_read >= 4096 ? 4096 : need_to_read;
                char buf[buf_size] = {0};
                ifs.read(buf, buf_size);
                int num = ifs.gcount();
                if (num != buf_size)
                {
                    LOG(INFO, "线程1没有读取到应该数量的数据");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    need_to_read -= buf_size;
                    int ret = send(sockfd_v[0], buf, buf_size, 0);
                    if (ret < 0)
                    {
                        LOG(FATAL, "套接字:%d有问题, %s", sockfd_v[0], strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    // if (ret > 0)
                    // {
                    //     LOG(INFO, "线程1成功向服务器发送了%d字节数据, 这和预期%d", ret, ret == buf_size);
                    // }
                }
            }
            ifs.close();
            LOG(INFO, "线程1给服务器发送文件%s中%d到%d的数据Success", filename.c_str(), 0, file_mid1);
            has_task_first_thread = false;
            std::lock_guard<std::mutex> guard(mtx1);
            task_finish--;
            if (task_finish == 0)
                cond_to_main_thread.notify_one();
            break;
        }
        }
    }
}
void ThreadFunc2()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx1);
        cond_to_other_thread.wait(lock, []()
                                  { return has_task_second_thread; });
        lock.unlock();
        LOG(INFO, "我是线程2, 现在有任务了");
        switch (task_opt)
        {
        case 1:
        {
            std::string upload_request = "Upload";
            upload_request = upload_request + " " + user_base_dir + filename + " " + std::to_string(file_mid1) + "-" + std::to_string(file_mid2) + "*.*";
            int ret = send(sockfd_v[1], upload_request.c_str(), upload_request.size(), 0);
            if (ret < 0)
            {
                LOG(FATAL, "套接字:%d出现问题%s, 发送请求:%s失败", sockfd_v[1], strerror(errno), upload_request.c_str());
                exit(EXIT_FAILURE);
            }
            std::ifstream ifs(filename.c_str() , std::ios_base::binary);
            int need_to_read = file_mid2 - file_mid1;
            LOG(INFO, "线程2需要给服务器发送文件%s中%d到%d的数据", filename.c_str(), file_mid1, file_mid2);
            ifs.seekg(file_mid1);
            if (ifs.rdstate() == std::ios_base::badbit)
            {
                LOG(FATAL, "线程2移动文件指针出错");
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
                    LOG(INFO, "线程2没有读取到应该数量的数据");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    need_to_read -= buf_size;
                    int ret = send(sockfd_v[1], buf, buf_size, 0);
                    if (ret < 0)
                    {
                        LOG(FATAL, "套接字:%d有问题, %s", sockfd_v[1], strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    // if (ret > 0)
                    // {
                    //     LOG(INFO, "线程2成功向服务器发送了%d字节数据, 这和预期%d", ret, ret == buf_size);
                    // }
                }
            }
            ifs.close();
            LOG(INFO, "线程2给服务器发送文件%s中%d到%d的数据Success", filename.c_str(), file_mid1, file_mid2);
            has_task_second_thread = false;
            std::lock_guard<std::mutex> guard(mtx1);
            task_finish--;
            if (task_finish == 0)
                cond_to_main_thread.notify_one();
            break;
        }
        }
    }
}
void ThreadFunc3()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx1);
        cond_to_other_thread.wait(lock, []()
                                  { return has_task_third_thread; });
        lock.unlock();
        LOG(INFO, "我是线程3, 现在有任务了");

        switch (task_opt)
        {
        case 1:
        {
            std::string upload_request = "Upload";
            upload_request = upload_request + " " + user_base_dir + filename + " " + std::to_string(file_mid2) + "-" + std::to_string(file_end) + "*.*";
            int ret = send(sockfd_v[2], upload_request.c_str(), upload_request.size(), 0);
            if (ret < 0)
            {
                LOG(FATAL, "套接字:%d出现问题%s, 发送请求:%s失败", sockfd_v[2], strerror(errno), upload_request.c_str());
                exit(EXIT_FAILURE);
            }
            std::ifstream ifs(filename.c_str() , std::ios_base::binary);
            int need_to_read = file_end - file_mid2;
            LOG(INFO, "线程3需要给服务器发送文件%s中%d到%d的数据", filename.c_str(), file_mid2, file_end);

            ifs.seekg(file_mid2);
            if (ifs.rdstate() == std::ios_base::badbit)
            {
                LOG(FATAL, "线程3移动文件指针出错");
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
                    LOG(INFO, "线程3没有读取到应该数量的数据");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    need_to_read -= buf_size;
                    int ret = send(sockfd_v[2], buf, buf_size, 0);
                    if (ret < 0)
                    {
                        LOG(FATAL, "套接字:%d有问题, %s", sockfd_v[2], strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    // if (ret > 0)
                    // {
                    //     LOG(INFO, "线程3成功向服务器发送了%d字节数据, 这和预期%d", ret, ret == buf_size);
                    // }
                }
            }
            ifs.close();
            LOG(INFO, "线程3给服务器发送文件%s中%d到%d的数据Success", filename.c_str(), file_mid2, file_end);
            has_task_third_thread = false;
            std::lock_guard<std::mutex> guard(mtx1);
            task_finish--;
            if (task_finish == 0)
                cond_to_main_thread.notify_one();
            break;
        }
        }
    }
}

void Menu()
{
    std::cout << "**************************************" << std::endl;
    std::cout << "Please select the corresponding number" << std::endl;
    std::cout << "*****0.Exit                      *****" << std::endl;
    std::cout << "*****1.Upload         2.Download *****" << std::endl;
    std::cout << "*****3.Delete         4.ShowFiles*****" << std::endl;
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
    std::cout << "文件大小是" << filesize << std::endl;
    if (filesize > 1024 * 10)
    {
        has_task_first_thread = true;
        has_task_second_thread = true;
        has_task_third_thread = true;

        task_opt = 1;
        file_mid1 = filesize / 3;
        file_mid2 = file_mid1 * 2;
        file_end = filesize;
        cond_to_other_thread.notify_all();
    }
    else
    {
        std::string upload_request = "Upload";
        upload_request = upload_request + " " + user_base_dir + filename + " " + "0" + "-" + std::to_string(filesize) + "*.*";
        if (send(sockfd_v[0], upload_request.c_str(), upload_request.size(), 0) == -1)
        {
            LOG(FATAL, "套接字:%d出现问题, %s", sockfd_v[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
        std::ifstream ifs(filename.c_str() , std::ios_base::binary);
        while (true)
        {
            char buf[4096] = {0};
            ifs.read(buf, 4096);
            int num = ifs.gcount();
            if (num == 0)
                break;
            else
            {
                int ret = send(sockfd_v[0], buf, num, 0);
                if (ret < 0)
                {
                    LOG(FATAL, "套接字:%d出现问题, %s", sockfd_v[0], strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
        }
        ifs.close();
        task_finish = 0;
        cond_to_main_thread.notify_one();
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
            std::cout << "请输入你要上传文件的文件名:";
            std::cin >> filename;
            while (HasFile(filename) == false)
            {
                std::cout << "请重新输入文件名:";
                std::cin >> filename;
            }
            task_finish = 3;
            UploadFunc(filename);
            std::unique_lock<std::mutex> lock(mtx2);
            cond_to_main_thread.wait(lock, []()
                                     { return task_finish == 0; });
            task_finish = 3;
            task_opt = -1;
            std::cout << "文件:" << filename << "上传成功" << std::endl;
            break;
        }
        case 2:
        {
            // 打印现有文件
            std::cout << "请输入你要下载文件的名称:";
            std::cin >> filename;
        }
        }
    }
}