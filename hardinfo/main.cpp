#include "cstdio"
#include "cstring"
#include "algorithm"
#include "cstdlib"
#include "unistd.h"
#include "hardinfo.h"
#include "diskdevice.h"
#include "meminfo.h"
#include "cpuinfo.h"
#include "pthread.h"
#define MAX_HARDINFO_LENGTH 1000
#define NUM_THREADS 10
using namespace std;
static pthread_t tids[NUM_THREADS];
int main(){
	char str[1000];
	int ret=pthread_create(&tids[0],NULL,cpuc_start,NULL);
	ret=pthread_create(&tids[1],NULL,diskc_start,NULL);
	ret=pthread_create(&tids[2],NULL,memc_start,NULL);
	printf("%d\n",ret);
	while(1){
		cpuc_get(str);
		printf("%s\n",str);
		diskc_get(str);
		printf("%s\n",str);
		memc_get(str);
		printf("%s\n",str);
		sleep(5);
	}
	pthread_exit(NULL);
	return 0;
}
