#include "../Log/Log.hpp"
#include "../TcpServer/Socket.hpp"
#include "../TcpServer/SocketBuffer.hpp"
#include "../TcpServer/EpollThread.hpp"
#include "../TcpServer/Acceptor.hpp"

int main()
{
    Acceptor acceptor(10);
    return 0;
}