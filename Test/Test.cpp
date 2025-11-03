#include <iostream>
#include <cstring>
#include <string>

int main()
{
    int num;
    std::string tmp(10, 0);
    std::cout << "请选择数字:";
    std::cin.getline(&tmp[0], 10);
    num = std::stoi(tmp);

    std::cout << "请输入文件名:";
    std::string filename;
    getline(std::cin, filename);
    std::cout << num << std::endl;
    std::cout << filename << std::endl;
    return 0;
}