#include<stdio.h>
#include<unistd.h>

int main(){
    printf("init start location\t0x%lx\n",sbrk(0));
    printf("init end location\t0x%lx\n",sbrk(0));
    sbrk(4*1024);
    printf("sbrk increased end location\t0x%lx\n",sbrk(0));

    int ans = brk(sbrk(0)+4*1024);
    if(ans==0)
        printf("brk increased end location\t0x%lx\n",sbrk(0));
    
    while(1);
}