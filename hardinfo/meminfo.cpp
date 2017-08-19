#include "cstdio"
#include "cstring"
#include "algorithm"
#include "cstdlib"
#include "memory.h"
#include "ctime"
#include "unistd.h"
#define MAX_CMD_LENGTH 1000
#define MAX_STR_LENGTH 1000
#define MAX_STR_LINE 10
#define VAR_NUMBER 5
#define FILE_PATH "./meminfo.txt"
using namespace std;
char meminfo_var_tab[20][100] = {
"总内存               ",
"空闲内存               ",
"可用内存            ",
"文件缓存            ",
"高速缓存            ",
 };
int last[VAR_NUMBER],now[VAR_NUMBER];
int memc_flag;
char memc_str[MAX_STR_LENGTH];
void meminfo_init(){
	memset(last,0,sizeof(last));
	memc_flag=1;
	memc_str[0]=0;
	return;
}
void mem_destroy(){
	memc_flag=0;
	memc_str[0]=0;
	return;
}
void get_meminfo_info(char *res){
	char cmd[MAX_CMD_LENGTH]="cp /proc/meminfo  /home/myt/graduate_design/hardinfo/meminfo.txt";
	char str[MAX_STR_LINE][MAX_STR_LENGTH+1];
	system(cmd);
	FILE *fp=NULL;
	fp=fopen(FILE_PATH,"r");
	if (fp==NULL){
		sprintf(res,"OPEN FILE FAIL!\n");
		return;
	}
	memset(now,0,sizeof(now));
	time_t timep;
	time(&timep);
	int offset=0,onset=0;
	offset+=sprintf(res+offset,"OK\n");
	offset+=sprintf(res+offset,"%s",ctime(&timep));
	for (int i=0;i<5;i++){
		fgets(str[i],MAX_STR_LENGTH,fp);
		char tmp[MAX_STR_LENGTH];
		sscanf(str[i],"%s%d",tmp,&now[i]);
		offset+=sprintf(res+offset,"%s:%10d\n",meminfo_var_tab[i],now[i]);
	}
	for (int i=0;i<5;i++) last[i]=now[i];
	fclose(fp);
	fp=NULL;
	return;
}
void* memc_start(void* args){
	meminfo_init();
	while(memc_flag){
		get_meminfo_info(memc_str);
		sleep(5);
	}
}
void memc_get(char *res){
	char *tmp;
	tmp=memc_str;
	while(*tmp!=0){
		*res=*tmp;
		tmp++;
		res++;
	}
	*res=0;
	return;
}
