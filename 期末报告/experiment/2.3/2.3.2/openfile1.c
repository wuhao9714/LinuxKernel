#include<stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(){
    int len = 32;
    char *filename = "./sharedfile";
    int fd = open(filename,O_RDWR );
    char *addr;
    addr = mmap(0, len, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

    char c = 'A';
    int i=0;
    while (1)
    {
        for(int j =0;j<len-1;j++){
             *(addr+j)=c;
        }
        i = (i+1)%26;
        c = 'A'+i;
    }

    return 0;
}