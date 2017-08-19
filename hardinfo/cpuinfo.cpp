#include "cstdio"
#include "cstring"
#include "algorithm"
#include "cstdlib"
#include "cpuinfo.h"
#include "ctime"
#include "unistd.h"
#define MAX_CMD_LENGTH 1000
#define MAX_STR_LENGTH 1000
#define VAR_NUMBER 4
#define FILE_PATH_1 "./stat1.txt"
#define FILE_PATH_2 "./stat2.txt"
using namespace std;
char cpuinfo_var_tab[20][100] = {
"CPU个数               ",
"CPU总使用率               ",
"号CPU使用率            ",
"CPU运行进程数            ",
"CPU阻塞进程数            "
 };
int cpu_last[20],cpu_now[20];
int cpuc_flag;
char cpuc_str[MAX_STR_LENGTH];
void cpuinfo_init(){
	cpuc_flag=1;
	memset(cpu_last,0,sizeof(cpu_last));
	memset(cpu_now,0,sizeof(cpu_now));
	return;
}
void cpuc_destroy(){
	cpuc_flag=0;
	cpuc_str[0]=0;
	return;
}
double get_cpu_use_rate(char *str1,char *str2){
	char tmp[MAX_STR_LENGTH];
	double sum1=0,sum2=0,idl1,idl2;
	int usr,nic,sys,idl,iow,irq,sof,ste,gue;
	sscanf(str1,"%s%d%d%d%d%d%d%d%d%d",tmp,&usr,&nic,&sys,&idl,&iow,&irq,&sof,&ste,&gue);
	sum1=usr+nic+sys+idl+iow+irq+sof+ste+gue;
	idl1=idl;
	sscanf(str2,"%s%d%d%d%d%d%d%d%d%d",tmp,&usr,&nic,&sys,&idl,&iow,&irq,&sof,&ste,&gue);
	sum2=usr+nic+sys+idl+iow+irq+sof+ste+gue;
	idl2=idl;
	//printf("%lf %lf %lf %lf\n",sum1,sum2,idl1,idl2);
	double res=100.0*(idl2-idl1)/(sum2-sum1);
	return res;
}
void get_cpuinfo_info(char *res){
	char cmd1[MAX_CMD_LENGTH]="cp /proc/stat  ./stat1.txt";
	system(cmd1);
	sleep(1);
	char cmd2[MAX_CMD_LENGTH]="cp /proc/stat  ./stat2.txt";
	system(cmd2);
	char str[2][MAX_STR_LENGTH+1];
	FILE *fp1=NULL,*fp2=NULL;
	fp1=fopen(FILE_PATH_1,"r");
	fp2=fopen(FILE_PATH_2,"r");
	if (fp1==NULL||fp2==NULL){
		sprintf(res,"OPEN FILE FAIL!\n");
		return;
	}
	memset(cpu_now,0,sizeof(cpu_now));
	time_t timep;
	time(&timep);
	int offset=0,onset=0;
	offset+=sprintf(res+offset,"OK\n");
	offset+=sprintf(res+offset,"%s",ctime(&timep));
	int CPU_NUM=sysconf(_SC_NPROCESSORS_CONF);
	offset+=sprintf(res+offset,"%s:%10d\n",cpuinfo_var_tab[0],cpu_now[0]=CPU_NUM);
	fgets(str[0],MAX_STR_LENGTH,fp1);
	fgets(str[1],MAX_STR_LENGTH,fp2);
	offset+=sprintf(res+offset,"%s:%10d%%\n",cpuinfo_var_tab[1],cpu_now[1]=100-int(get_cpu_use_rate(str[0],str[1])));
	for (int i=0;i<CPU_NUM;i++){
		fgets(str[0],MAX_STR_LENGTH,fp1);
		fgets(str[1],MAX_STR_LENGTH,fp2);
		offset+=sprintf(res+offset,"%d%s:%10d%%\n",i,cpuinfo_var_tab[2],cpu_now[2+i]=100-int(get_cpu_use_rate(str[0],str[1])));
	}
	memcpy(cpu_last,cpu_now,sizeof(cpu_last));
	fclose(fp1);
	fclose(fp2);
	fp1=NULL;
	fp2=NULL;
	return;
}
void* cpuc_start(void* args){
	cpuinfo_init();
	while(cpuc_flag){
		get_cpuinfo_info(cpuc_str);
		sleep(5);
	}
}
void cpuc_get(char *res){
	char *tmp;
	tmp=cpuc_str;
	while(*tmp!=0){
		*res=*tmp;
		tmp++;
		res++;
	}
	*res=0;
	return;
}
