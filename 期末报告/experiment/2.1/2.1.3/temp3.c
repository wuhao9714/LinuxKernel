#include <stdio.h>
int main(){
    void *small = (void *)malloc(128*1024);
    void *big = (void *)malloc(128*1024+1);
    printf("small\t location: 0x%lx\n", small);
    printf("big\t location: 0x%lx\n", big);
    while(1);
    return 0;
}