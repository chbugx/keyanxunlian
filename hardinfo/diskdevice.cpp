#include "cstdio"
#include "cstring"
#include "algorithm"
#include "cstdlib"
#include "diskdevice.h"
#include "ctime"
#include "unistd.h"
#define MAX_CMD_LENGTH 1000
#define MAX_STR_LENGTH 1000
#define VAR_NUMBER 4
#define FILE_PATH "./diskdevice.txt"
using namespace std;
char diskdevice_var_tab[20][100] = {
"磁盘读完成次数               ",
"读花费毫秒数               ",
"磁盘写完成次数            ",
"写花费毫秒数            "
 };
int last_rd,last_rt,last_wd,last_wt;
int now_rd,now_rt,now_wd,now_wt;
int diskc_flag;
char diskc_str[MAX_STR_LENGTH];
void diskdevice_init(){
	diskc_flag=1;
	last_rd=last_rt=last_wd=last_wt=0;
	return;
}
void diskc_destroy(){
	diskc_flag=0;
	diskc_str[0]=0;
	return;
}
void get_diskdevice_info(char *res){
	char cmd[MAX_CMD_LENGTH]="cp /proc/diskstats  /home/myt/graduate_design/hardinfo/diskdevice.txt";
	char str[MAX_STR_LENGTH+1];
	system(cmd);
	FILE *fp=NULL;
	fp=fopen(FILE_PATH,"r");
	if (fp==NULL){
		sprintf(res,"OPEN FILE FAIL!\n");
		return;
	}
	now_rd=now_rt=now_wd=now_wt=0;
	time_t timep;
	time(&timep);
	int offset=0,onset=0;
	offset+=sprintf(res+offset,"OK\n");
	offset+=sprintf(res+offset,"%s",ctime(&timep));
	while(fgets(str,MAX_STR_LENGTH,fp)!=NULL){
		int tmp_int,a,b,c,d;
		char tmp_str[100];
		sscanf(str,"%d%d%s%d%d%d%d%d%d%d%d",&tmp_int,&tmp_int,tmp_str,&a,&tmp_int,&tmp_int,&b,&c,&tmp_int,&tmp_int,&d);
		now_rd+=a;
		now_rt+=b;
		now_wd+=c;
		now_wt+=d;
	}
	offset+=sprintf(res+offset,"%s:%10d\n",diskdevice_var_tab[0],now_rd-last_rd);
	offset+=sprintf(res+offset,"%s:%10d\n",diskdevice_var_tab[1],now_rt-last_rt);
	offset+=sprintf(res+offset,"%s:%10d\n",diskdevice_var_tab[2],now_wd-last_wd);
	offset+=sprintf(res+offset,"%s:%10d\n",diskdevice_var_tab[3],now_wt-last_wt);
	last_rd=now_rd;
	last_rt=now_rt;
	last_wd=now_wd;
	last_wt=now_wt;
	fclose(fp);
	fp=NULL;
	return;
}
void* diskc_start(void* args){
	diskdevice_init();
	while(diskc_flag){
		get_diskdevice_info(diskc_str);
		sleep(5);
	}
}
void diskc_get(char *res){
	char *tmp;
	tmp=diskc_str;
	while(*tmp!=0){
		*res=*tmp;
		tmp++;
		res++;
	}
	*res=0;
	return;
}
