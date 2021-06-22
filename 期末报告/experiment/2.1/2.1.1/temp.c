#include <stdio.h>
int a = 0x0A0B0C0D;
int main(){

    printf("a \tvalue:%d\t location: 0x%lx\n",a, &a);
    printf("location: 0x%lx\tvalue:0x%lx\n",(char*)&a,*((char*)&a));
    printf("location: 0x%lx\tvalue:0x%lx\n",(char*)&a+1,*((char*)&a+1));
    printf("location: 0x%lx\tvalue:0x%lx\n",(char*)&a+2,*((char*)&a+2));
    printf("location: 0x%lx\tvalue:0x%lx\n",(char*)&a+3,*((char*)&a+3));
    return 0;
}