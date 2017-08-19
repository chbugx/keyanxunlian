#include "cstdio"
#include "cstring"
#include "algorithm"
#include "cstdlib"
#include "hardinfo.h"
#include "ctime"
#define MAX_CMD_LENGTH 1000
#define MAX_STR_LENGTH 1000
#define VAR_NUMBER 16
#define FILE_PATH "./vmstat.txt"
using namespace std;
char var_tab[20][100] = {
"CPU运行进程数               ",
"CPU阻塞进程数               ",
"虚拟内存使用大小            ",
"空闲物理内存大小            ",
"系统缓存                    ",
"Cache                       ",
"每秒从磁盘读入虚拟内存的大小",
"每秒虚拟内存写入磁盘的大小  ",
"块设备每秒接收的块数量      ",
"块设备每秒发送的块数量      ",
"每秒CPU的中断次数           ",
"每秒上下文切换次数          ",
"用户CPU时间                 ",
"系统CPU时间                 ",
"空闲 CPU时间                ",
"等待IO CPU时间              "
 };

void get_hardinfo(char *res){
	char cmd[MAX_CMD_LENGTH]="vmstat > vmstat.txt";
	char str[MAX_STR_LENGTH+1];
	system(cmd);
	FILE *fp=NULL;
	fp=fopen(FILE_PATH,"r");
	if (fp==NULL){
		sprintf(res,"OPEN FILE FAIL!\n");
		return;
	}
	time_t timep;
	time(&timep);
	int offset=0,onset=0;
	offset+=sprintf(res+offset,"OK\n");
	offset+=sprintf(res+offset,"%s",ctime(&timep));
	fgets(str,MAX_STR_LENGTH,fp);
	fgets(str,MAX_STR_LENGTH,fp);
	for (int i=0;i<VAR_NUMBER;i++){
		char tmp[MAX_STR_LENGTH];
		fscanf(fp,"%s",tmp);
		offset+=sprintf(res+offset,"%s:%10s\n",var_tab[i],tmp);
	}
	fclose(fp);
	fp=NULL;
	return;
}
