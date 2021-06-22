#include <stdio.h>

#include <stdlib.h>

//未初始化：u 初始化为零：o　初始化为非零：i
//静态：sta　常量：con　局部：loc　全局：glo


//全局
int u_glo1;
int o_glo1 = 0;
int i_glo1 =1;
//静态－全局
static int u_sta_glo1;
static int o_sta_glo1 = 0;
static int i_sta_glo1 =1;
//常量－全局
const int u_con_glo1;
const int o_con_glo1 = 0;
const int i_con_glo1 =1;
//静态－常量－全局
static const int u_sta_con_glo1;
static const int o_sta_con_glo1 = 0;
static const int i_sta_con_glo1 =1;


int main(){
//局部
int u_loc1;
int o_loc1 = 0;
int i_loc1 =1;
//静态－局部
static int u_sta_loc1;
static int o_sta_loc1 = 0;
static int i_sta_loc1 =1;
//常量－局部
const int u_con_loc1;
const int o_con_loc1 = 0;
const int i_con_loc1 =1;
//静态－常量－局部
static const int u_sta_con_loc1;
static const int o_sta_con_loc1 = 0;
static const int i_sta_con_loc1 =1;

  // char *localvar1 = (char *)malloc(sizeof(char));


  printf("u_glo1 \tvalue:%d\t location: 0x%lx\n",u_glo1, &u_glo1);
  printf("o_glo1 \tvalue:%d\t location: 0x%lx\n",o_glo1, &o_glo1);
  printf("i_glo1 \tvalue:%d\t location: 0x%lx\n",i_glo1, &i_glo1);
  printf("\n");
  printf("u_sta_glo1 \tvalue:%d\t location: 0x%lx\n",u_sta_glo1, &u_sta_glo1);
  printf("o_sta_glo1 \tvalue:%d\t location: 0x%lx\n",o_sta_glo1, &o_sta_glo1);
  printf("i_sta_glo1 \tvalue:%d\t location: 0x%lx\n",i_sta_glo1, &i_sta_glo1);
  printf("\n");
  printf("u_con_glo1 \tvalue:%d\t location: 0x%lx\n",u_con_glo1, &u_con_glo1);
  printf("o_con_glo1 \tvalue:%d\t location: 0x%lx\n",o_con_glo1, &o_con_glo1);
  printf("i_con_glo1 \tvalue:%d\t location: 0x%lx\n",i_con_glo1, &i_con_glo1);
  printf("\n");
  printf("u_sta_con_glo1 \tvalue:%d\t location: 0x%lx\n",u_sta_con_glo1, &u_sta_con_glo1);
  printf("o_sta_con_glo1 \tvalue:%d\t location: 0x%lx\n",o_sta_con_glo1, &o_sta_con_glo1);
  printf("i_sta_con_glo1 \tvalue:%d\t location: 0x%lx\n",i_sta_con_glo1, &i_sta_con_glo1);
  printf("\n");
  printf("u_loc1 \tvalue:%d\t location: 0x%lx\n",u_loc1, &u_loc1);
  printf("o_loc1 \tvalue:%d\t location: 0x%lx\n",o_loc1, &o_loc1);
  printf("i_loc1 \tvalue:%d\t location: 0x%lx\n",i_loc1, &i_loc1);
  printf("\n");
  printf("u_sta_loc1 \tvalue:%d\t location: 0x%lx\n",u_sta_loc1, &u_sta_loc1);
  printf("o_sta_loc1 \tvalue:%d\t location: 0x%lx\n",o_sta_loc1, &o_sta_loc1);
  printf("i_sta_loc1 \tvalue:%d\t location: 0x%lx\n",i_sta_loc1, &i_sta_loc1);
  printf("\n");
  printf("u_con_loc1 \tvalue:%d\t location: 0x%lx\n",u_con_loc1, &u_con_loc1);
  printf("o_con_loc1 \tvalue:%d\t location: 0x%lx\n",o_con_loc1, &o_con_loc1);
  printf("i_con_loc1 \tvalue:%d\t location: 0x%lx\n",i_con_loc1, &i_con_loc1);
  printf("\n"); 
  printf("u_sta_con_loc1 \tvalue:%d\t location: 0x%lx\n",u_sta_con_loc1, &u_sta_con_loc1);
  printf("o_sta_con_loc1 \tvalue:%d\t location: 0x%lx\n",o_sta_con_loc1, &o_sta_con_loc1);
  printf("i_sta_con_loc1 \tvalue:%d\t location: 0x%lx\n",i_sta_con_loc1, &i_sta_con_loc1);
  
  while(1);

  return(0);

}


