#include "../Log/Log.hpp"
#include "../TcpServer/Util.hpp"
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
#include <cstring>

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
bool exit_sign = false;
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
                                  { return has_task_first_thread || exit_sign; });
        lock.unlock();
        if (exit_sign == true)
            exit(EXIT_SUCCESS);
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
            std::ifstream ifs(filename.c_str(), std::ios_base::binary);
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
        case 2:
        {
            std::string request = std::string("Download") + " " + user_base_dir + filename + " " + "0" + "-" + std::to_string(file_mid1) + "*.*";
            int ret = send(sockfd_v[0], request.c_str(), request.size(), 0);
            if (ret < 0)
            {
                LOG(FATAL, "send失败, 套接字%d, %s", sockfd_v[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
            int tmp = file_mid1;
            std::ofstream ofs(filename, ofs.in | ofs.out | ofs.binary);
            if (ofs.is_open() == false)
            {
                LOG(FATAL, "打开文件:%s失败", filename.c_str());
                exit(EXIT_FAILURE);
            }
            while (tmp != 0)
            {
                // std::cout << "线程1" << "tmp:" << tmp << std::endl;
                const int buf_size = tmp >= 4096 ? 4096 : tmp;
                char recv_buf[buf_size] = {0};
                // LOG(INFO, "我是线程1, 我要recv了");
                int ret = recv(sockfd_v[0], recv_buf, buf_size, 0);
                // LOG(INFO, "我是线程1, recv返回了, ret:%d", ret);
                if (ret < 0)
                {
                    LOG(FATAL, "recv失败, 套接字%d, %s", sockfd_v[0], strerror(errno));
                    exit(EXIT_FAILURE);
                }
                else if (ret == 0)
                {
                    LOG(FATAL, "对方要关闭连接, 这不正常, 套接字%d", sockfd_v[0]);
                    exit(EXIT_FAILURE);
                }
                else
                {
                    tmp -= ret;
                    ofs.write(recv_buf, ret);
                    if (ofs.rdstate() == ofs.badbit)
                    {
                        LOG(FATAL, "向文件%s写入数据失败", filename.c_str());
                        exit(EXIT_FAILURE);
                    }
                }
            }
            ofs.close();
            // LOG(INFO, "线程1成功接受文件:%s, %d到%d的内容", filename.c_str(), 0, file_mid1);
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
                                  { return has_task_second_thread || exit_sign; });
        lock.unlock();
        if (exit_sign == true)
            exit(EXIT_SUCCESS);
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
            std::ifstream ifs(filename.c_str(), std::ios_base::binary);
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
        case 2:
        {
            std::string request = std::string("Download") + " " + user_base_dir + filename + " " + std::to_string(file_mid1) + "-" + std::to_string(file_mid2) + "*.*";
            int ret = send(sockfd_v[1], request.c_str(), request.size(), 0);
            if (ret < 0)
            {
                LOG(FATAL, "send失败, 套接字%d, %s", sockfd_v[1], strerror(errno));
                exit(EXIT_FAILURE);
            }
            int tmp = file_mid2 - file_mid1;
            std::ofstream ofs(filename, ofs.in | ofs.out | ofs.binary);
            if (ofs.is_open() == false)
            {
                LOG(FATAL, "打开文件:%s失败", filename.c_str());
                exit(EXIT_FAILURE);
            }
            ofs.seekp(file_mid1);
            if (ofs.rdstate() == ofs.failbit)
            {
                LOG(FATAL, "移动文件指针到%d失败", file_mid1);
                exit(EXIT_FAILURE);
            }
            while (tmp != 0)
            {
                // std::cout << "线程2" << "tmp:" << tmp << std::endl;
                const int buf_size = tmp >= 4096 ? 4096 : tmp;
                char recv_buf[buf_size] = {0};
                // LOG(INFO, "我是线程2我要开始recv了");
                int ret = recv(sockfd_v[1], recv_buf, buf_size, 0);
                // LOG(INFO, "我是线程2, recv返回了, ret:%d", ret);
                if (ret < 0)
                {
                    LOG(FATAL, "recv失败, 套接字%d, %s", sockfd_v[1], strerror(errno));
                    exit(EXIT_FAILURE);
                }
                else if (ret == 0)
                {
                    LOG(FATAL, "对方要关闭连接, 这不正常, 套接字%d", sockfd_v[1]);
                    exit(EXIT_FAILURE);
                }
                else
                {
                    tmp -= ret;
                    ofs.write(recv_buf, ret);
                    if (ofs.rdstate() == ofs.badbit)
                    {
                        LOG(FATAL, "向文件%s写入数据失败", filename.c_str());
                        exit(EXIT_FAILURE);
                    }
                }
            }
            ofs.close();
            // LOG(INFO, "线程2成功接受文件:%s, %d到%d的内容", filename.c_str(), file_mid1, file_mid2);
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
                                  { return has_task_third_thread || exit_sign; });
        lock.unlock();
        if (exit_sign == true)
            exit(EXIT_SUCCESS);
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
            std::ifstream ifs(filename.c_str(), std::ios_base::binary);
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
        case 2:
        {
            std::string request = std::string("Download") + " " + user_base_dir + filename + " " + std::to_string(file_mid2) + "-" + std::to_string(file_end) + "*.*";
            int ret = send(sockfd_v[2], request.c_str(), request.size(), 0);
            if (ret < 0)
            {
                LOG(FATAL, "send失败, 套接字%d, %s", sockfd_v[2], strerror(errno));
                exit(EXIT_FAILURE);
            }
            int tmp = file_end - file_mid2;
            std::ofstream ofs(filename, ofs.in | ofs.out | ofs.binary);
            if (ofs.is_open() == false)
            {
                LOG(FATAL, "打开文件:%s失败", filename.c_str());
                exit(EXIT_FAILURE);
            }
            ofs.seekp(file_mid2);
            if (ofs.rdstate() == ofs.failbit)
            {
                LOG(FATAL, "移动文件指针到%d失败", file_mid2);
                exit(EXIT_FAILURE);
            }
            while (tmp != 0)
            {
                // std::cout << "线程3" << "tmp:" << tmp << std::endl;
                const int buf_size = tmp >= 4096 ? 4096 : tmp;
                char recv_buf[buf_size] = {0};
                // LOG(INFO, "我是线程3我要recv了");
                int ret = recv(sockfd_v[2], recv_buf, buf_size, 0);
                // LOG(INFO, "我是线程3, recv返回了, ret:%d", ret);
                if (ret < 0)
                {
                    LOG(FATAL, "recv失败, 套接字%d, %s", sockfd_v[2], strerror(errno));
                    exit(EXIT_FAILURE);
                }
                else if (ret == 0)
                {
                    LOG(FATAL, "对方要关闭连接, 这不正常, 套接字%d", sockfd_v[2]);
                    exit(EXIT_FAILURE);
                }
                else
                {
                    tmp -= ret;
                    ofs.write(recv_buf, ret);
                    if (ofs.rdstate() == ofs.badbit)
                    {
                        LOG(FATAL, "向文件%s写入数据失败", filename.c_str());
                        exit(EXIT_FAILURE);
                    }
                }
            }
            ofs.close();
            // LOG(INFO, "线程3成功接受文件:%s, %d到%d的内容", filename.c_str(), file_mid2, file_end);
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
    std::cout << "*****0.Exit                      *****" << std::endl;
    std::cout << "*****1.Upload         2.Download *****" << std::endl;
    std::cout << "*****3.Delete         4.ShowFiles*****" << std::endl;
    std::cout << "**************************************" << std::endl;
    std::cout << "请输入功能前面对应的数字:";
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

void ExitFunc()
{
    {
        std::lock_guard<std::mutex> guard(mtx1);
        exit_sign = true;
    }
    cond_to_other_thread.notify_all();

    for (auto e : sockfd_v)
    {
        if (close(e) == -1)
        {
            LOG(FATAL, "关闭套接字失败:%d, %s", e, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    std::cout << "下次见😄" << std::endl;
    exit(EXIT_SUCCESS);
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
        std::ifstream ifs(filename.c_str(), std::ios_base::binary);
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
void ShowFilesFunc()
{
    std::string request = std::string("ShowFiles") + " " + user_base_dir + " " + "0" + "-" + "0" + "*.*";
    int ret = send(sockfd_v[0], request.c_str(), request.size(), 0);
    if (ret < 0)
    {
        LOG(INFO, "send向套接字%d发送数据失败, %s", sockfd_v[0], strerror(errno));
        exit(EXIT_FAILURE);
    }
    std::string buf;
    std::string each_recv(40960, 0);

    ret = recv(sockfd_v[0], &each_recv[0], 40960, 0);
    if (ret < 0)
    {
        LOG(INFO, "recv失败, %s, 套接字%d", strerror(errno), sockfd_v[0]);
        exit(EXIT_FAILURE);
    }
    else
        buf.insert(buf.begin(), each_recv.begin(), each_recv.begin() + ret);
    std::cout << "当前的文件有:" << std::endl;
    std::cout << buf;
}
void DownloadFunc(int file_size)
{
    if (file_size < 1024 * 10)
    {
        std::string request = std::string("Download") + " " + user_base_dir + filename + " " + "0" + "-" + std::to_string(file_size) + "*.*";
        int ret = send(sockfd_v[0], request.c_str(), request.size(), 0);
        if (ret < 0)
        {
            LOG(FATAL, "send失败, 套接字%d, %s", sockfd_v[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
        int tmp = file_size;
        std::ofstream ofs(filename, ofs.in | ofs.out | ofs.binary);
        if (ofs.is_open() == false)
        {
            LOG(FATAL, "打开文件:%s失败", filename.c_str());
            exit(EXIT_FAILURE);
        }
        while (tmp != 0)
        {
            const int buf_size = tmp >= 4096 ? 4096 : tmp;
            char recv_buf[buf_size] = {0};
            int ret = recv(sockfd_v[0], recv_buf, buf_size, 0);
            if (ret < 0)
            {
                LOG(FATAL, "recv失败, 套接字%d, %s", sockfd_v[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
            else if (ret == 0)
            {
                LOG(FATAL, "对方要关闭连接, 这不正常, 套接字%d", sockfd_v[0]);
                exit(EXIT_FAILURE);
            }
            else
            {
                tmp -= ret;
                ofs.write(recv_buf, ret);
                if (ofs.rdstate() == ofs.badbit)
                {
                    LOG(FATAL, "向文件%s写入数据失败", filename.c_str());
                    exit(EXIT_FAILURE);
                }
            }
        }
        ofs.close();
        task_finish = 0;
        cond_to_main_thread.notify_one();
    }
    else
    {
        has_task_first_thread = true;
        has_task_second_thread = true;
        has_task_third_thread = true;

        task_opt = 2;
        file_mid1 = file_size / 3;
        file_mid2 = file_mid1 * 2;
        file_end = file_size;
        cond_to_other_thread.notify_all();
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
    for (auto &e : thread_v)
        e.detach();

    while (true)
    {
        int input = 0;
        Menu();
        std::string tmp;
        getline(std::cin, tmp);
        try
        {
            input = std::stoi(tmp);
        }
        catch (std::invalid_argument const &ex)
        {
            std::cout << "输入不正确, 请重新输入!" << std::endl;
            continue;
        }
        while (!(input >= 0 && input <= 4))
        {
            std::cout << "输入不正确, 请重新输入正确的数字:";
            getline(std::cin, tmp);
            try
            {
                input = std::stoi(tmp);
            }
            catch (std::invalid_argument const &ex)
            {
                continue;
            }
        }

        switch (input)
        {
        case 0:
        {
            ExitFunc();
            break;
        }
        case 1:
        {
            std::cout << "请输入你要上传文件的文件名:";
            getline(std::cin, filename);
            while (HasFile(filename) == false)
            {
                std::cout << "请重新输入文件名:";
                getline(std::cin, filename);
            }
            task_finish = 3;
            UploadFunc(filename);
            std::unique_lock<std::mutex> lock(mtx2);
            cond_to_main_thread.wait(lock, []()
                                     { return task_finish == 0; });
            std::cout << "文件:" << filename << "上传成功" << std::endl;
            break;
        }
        case 2:
        {
            ShowFilesFunc();
            std::cout << "请输入你要下载文件的名称:";
            getline(std::cin, filename);
            int file_exist = 0, file_size = 0;
            while (true)
            {
                std::string request = std::string("Check") + " " + user_base_dir + filename + " " + "0-0" + "*.*";
                int ret = send(sockfd_v[0], request.c_str(), request.size(), 0);
                if (ret < 0)
                {
                    LOG(INFO, "send失败, 套接字%d, %s", sockfd_v[0], strerror(errno));
                    exit(EXIT_FAILURE);
                }
                std::string recv_buf(100, 0);
                ret = recv(sockfd_v[0], &recv_buf[0], recv_buf.size(), 0);
                if (ret < 0)
                {
                    LOG(INFO, "recv失败, 套接字%d, %s", sockfd_v[0], strerror(errno));
                    exit(EXIT_FAILURE);
                }
                file_exist = ResponseUtil::ParseForFileExist(recv_buf);
                file_size = ResponseUtil::ParseForFileSize(recv_buf);
                if (file_exist == 0)
                {
                    std::cout << "云盘中没有这个文件, 请重新输入文件名:";
                    getline(std::cin, filename);
                    continue;
                }
                else
                {
                    std::ofstream ofs(filename, ofs.binary | ofs.trunc);
                    if (ofs.is_open() == false)
                    {
                        LOG(FATAL, "创建文件%s失败", filename.c_str());
                        exit(EXIT_FAILURE);
                    }
                    ofs.close();
                    std::cout << "正在下载文件请稍等, 这个过程请不要退出客户端" << std::endl;
                    break;
                }
            }
            task_finish = 3;
            DownloadFunc(file_size);
            std::unique_lock<std::mutex> lock(mtx2);
            cond_to_main_thread.wait(lock, []()
                                     { return task_finish == 0; });
            std::cout << "文件:" << filename << "下载成功" << std::endl;
            break;
        }
        case 3:
        {
            ShowFilesFunc();
            std::cout << "请输入你要删除的文件名:";
            getline(std::cin, filename);
            int file_exist = 0;
            while (true)
            {
                std::string request = std::string("Check") + " " + user_base_dir + filename + " " + "0-0" + "*.*";
                int ret = send(sockfd_v[0], request.c_str(), request.size(), 0);
                if (ret < 0)
                {
                    LOG(INFO, "send失败, 套接字%d, %s", sockfd_v[0], strerror(errno));
                    exit(EXIT_FAILURE);
                }
                std::string recv_buf(100, 0);
                ret = recv(sockfd_v[0], &recv_buf[0], recv_buf.size(), 0);
                if (ret < 0)
                {
                    LOG(INFO, "recv失败, 套接字%d, %s", sockfd_v[0], strerror(errno));
                    exit(EXIT_FAILURE);
                }
                file_exist = ResponseUtil::ParseForFileExist(recv_buf);
                if (file_exist == 0)
                {
                    std::cout << "云盘中没有这个文件" << std::endl;
                    break;
                }
                else
                {
                    request = std::string("Delete") + " " + user_base_dir + filename + " " + "0-0" + "*.*";
                    ret = send(sockfd_v[0], request.c_str(), request.size(), 0);
                    if (ret < 0)
                    {
                        LOG(INFO, "send失败, 套接字%d, %s", sockfd_v[0], strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        std::cout << "已提交删除任务" << std::endl;
                        break;
                    }
                }
            }
            break;
        }
        case 4:
        {
            ShowFilesFunc();
            break;
        }
        }
    }
}