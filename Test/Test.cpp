#include "../CloudStorage/Log/Log.hpp"
#include "../CloudStorage/TcpServer/SocketBuffer.hpp"


int main()
{
    // Buffer buf;
    // std::string str = "hello world";
    // buf.WriteInBuffer(str.size(), str.c_str());
    // std::string text = buf.ReadFromBuffer();
    // std::cout << text << std::endl;

    Buffer buf;
    for(int i = 0; i <= 1029; i++)
    {
        buf.WriteInBuffer(1, "w");
    }

    std::cout << buf.WriteAbleSize() << std::endl;

    std::cout << buf.ReadAbleSize() << std::endl;
    buf.ReadFromBuffer();
    std::cout << buf.ReadAbleSize() << std::endl;
    return 0;
}