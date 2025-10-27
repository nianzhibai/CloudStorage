#include "../CloudStorage/TcpServer/Buffer.hpp"
#include <iostream>

int main()
{
    // Buffer buf;
    // std::string tmp = "hello world";
    // std::cout << buf.ReadAbleSize() << std::endl;
    // buf.WriteInBuffer(tmp.size(), tmp.c_str());
    // std::cout << buf.ReadAbleSize() << std::endl;
    // buf.ReadAllFromBuffer();
    // std::cout << buf.ReadAbleSize() << std::endl;

    Buffer buf;
    std::cout << buf.Capacity() << std::endl;
    for(int i = 0; i < 1025; i++)
    {
        buf.WriteInBuffer(1, "h");
    }
    std::cout << buf.ReadAbleSize() << std::endl;
    std::cout << buf.Capacity() << std::endl;
    buf.ReadAllFromBuffer();
    std::cout << buf.ReadAbleSize() << std::endl;
    std::cout << buf.Capacity() << std::endl;
    return 0;
}