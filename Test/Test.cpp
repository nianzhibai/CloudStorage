#include "../CloudStorage/Log/Log.hpp"
#include <iostream>
#include <sys/stat.h>

int main()
{
    struct stat statbuf; 
    if(stat("./test.txt", &statbuf) == -1)
    {
        std::cout << EACCES << std::endl;
        std::cout << errno << std::endl;
    }
    return 0;
}