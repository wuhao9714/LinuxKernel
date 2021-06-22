#include <fcntl.h>

#include <stdio.h>

#include <stdlib.h>

int globalvar1;

int globalvar2 = 3;

void mylocalfoo()

{

  int functionvar;

  printf("variable functionvar \t location: 0x%lx\n", &functionvar);

}

int main()

{

  void *localvar1 = (void *)malloc(2048);

  printf("variable globalvar1 \t location: 0x%lx\n", &globalvar1);

  printf("variable globalvar2 \t location: 0x%lx\n", &globalvar2);

  printf("variable localvar1 \t location: 0x%lx\n", &localvar1);

  mylibfoo();

  mylocalfoo();

  while(1);

  return(0);

}


