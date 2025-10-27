#include "../CloudStorage/Log/Log.hpp"
#include <iostream>
#include <sys/stat.h>
#include <fstream>
#include "../CloudStorage/TcpServer/Util.hpp"

int main()
{
    // std::ofstream ofs("test.txt", std::ios_base::binary);
    // ofs.seekp(10);
    // if (ofs.rdstate() == std::ios_base::failbit)
    // {
    //     std::cout << "移动文件指针失败" << std::endl;
    //     exit(EXIT_FAILURE);
    // }
    // ofs.write("hello", 5);
    // if (ofs.rdstate() == std::ios_base::failbit)
    // {
    //     std::cout << "写入文件失败" << std::endl;
    //     exit(EXIT_FAILURE);
    // }
    // std::string pathname = "./Hello";
    // if(mkdir(pathname.c_str(), 0777) == -1)
    // {
    //     std::cout << "创建目录失败:" << strerror(errno) << std::endl;
    //     exit(EXIT_FAILURE);
    // }

    // // 目录mkdir创建不能嵌套
    // std::string filename = "./Hello/helloworld.txt";
    // FileNameOk(filename);
    std::string filename = "./You/And/Me/youandme.txt";
    FileUtil::FileNameOk(filename);

    return 0;
}