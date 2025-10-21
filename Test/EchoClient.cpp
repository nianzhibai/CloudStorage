#include "../CloudStorage/TcpServer/Socket.hpp"

int main()
{
    Socket sock(false);
    sock.Connect("127.0.0.1", 1025);


    char buffer[1024] = { 0 };
    std::string tmp = "你好";
    for(int i = 0; i < 30; i++)
    {
        int ret = send(sock.GetFd(), tmp.c_str(), tmp.size(), 0);
        ret = recv(sock.GetFd(), buffer, 1024, 0);
        
        buffer[ret] = 0;
        std::cout << buffer << std::endl;
        sleep(1);
    }
}