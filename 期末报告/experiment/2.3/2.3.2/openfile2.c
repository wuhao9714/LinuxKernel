#include<stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(){
    int len = 32;
    char *filename = "./sharedfile";
    int fd = open(filename,O_RDWR );
    char *addr;
    addr = mmap(0, len, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

    while (1)
    {
        printf("%s",addr);
    }

    return 0;
}