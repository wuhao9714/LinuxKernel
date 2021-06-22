#include <stdio.h>
#include <unistd.h>  //封装系统API接口

int main()
{
    long pfcount = syscall(2020);
    printf("pfcount: %ld\n", pfcount);
    return 0;
}