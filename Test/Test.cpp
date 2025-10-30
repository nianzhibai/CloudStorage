#include <iostream>
#include <fstream>
#include <string>

int main()
{
    std::ofstream ofs("test.txt", std::ios_base::binary | std::ios_base::out | std::ios_base::in);
    ofs.seekp(100);
    if (ofs.rdstate() != ofs.goodbit)
    {
        std::cout << "移动文件指针失败" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string str = "hello world!";
    ofs.write(str.c_str(), str.size());
    if (ofs.rdstate() != ofs.goodbit)
    {
        std::cout << "写入文件有问题" << std::endl;
        exit(EXIT_FAILURE);
    }
    ofs.close();

    str = "you are";
    ofs.open("test.txt", std::ios_base::binary | std::ios_base::out | std::ios_base::in);
    ofs.seekp(0);
    if (ofs.rdstate() != ofs.goodbit)
    {
        std::cout << "移动文件指针失败" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << ofs.tellp() << std::endl;
    ofs.write(str.c_str(), str.size());
    if (ofs.rdstate() != ofs.goodbit)
    {
        std::cout << "写入文件有问题" << std::endl;
        exit(EXIT_FAILURE);
    }
    ofs.close();

    return 0;
}