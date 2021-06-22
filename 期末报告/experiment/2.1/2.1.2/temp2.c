#include <stdio.h>
char *strglo = "ABC";
int main(){
    char *strloc = "abc";
    printf("\"ABC\"\t location: 0x%lx\n", strglo);
    printf("\"abc\"\t location: 0x%lx\n", strloc);
    while(1);
    return 0;
}