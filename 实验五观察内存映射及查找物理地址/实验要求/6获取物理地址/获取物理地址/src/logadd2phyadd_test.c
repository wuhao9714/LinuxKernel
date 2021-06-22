#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>
#define BUFSIZE 4096
char buf[BUFSIZE];
typedef struct data {
	unsigned long addr;
	int p;
}mydata;

int main() 
{ 
	unsigned long tmp,addr;
	int fd,len;
	mydata wdata;
	tmp = 0x12345678;
	addr = &tmp;
    //输出tmp?
	//输出addr?
	wdata.addr = addr;
	wdata.p = getpid();
	//输出进程号pid?
    //打开文件
	fd = open("/proc/logadd2phyadd",O_RDWR);
    //将wdata写入文件?
    //读文件?
    //输出文件长度及内容
    printf("the read length is %d and the buf content is: \n%s\n",len,buf);

	sleep(2);
	close(fd);
	return 0;
} 

