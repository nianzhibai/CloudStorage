#include <iostream>
#include <cstring>
#include <fcntl.h>

int main()
{
    if(open("./nihc.txt", O_RDONLY) == -1)
    {
        if(errno == EACCES)
        {
            std::cout << "文件不存在" << std::endl;
        }
        std::cout << "ENOENT:" << ENOENT << std::endl; 
        std::cout << "EACCES:" << EACCES << std::endl; 
        std::cout << errno << strerror(errno) << std::endl;
    }
    return 0;
}