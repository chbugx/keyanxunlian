#include "cstdio"
#include "cstring"
#include "algorithm"
#include "cstdlib"
#include "cpuinfo.h"
#include "ctime"
#include "unistd.h"
using namespace std;
int main(){
	FILE *fp=fopen("./test","r");
	fseek(fp,4,0);
	char ch;
	fscanf(fp,"%c",&ch);
	printf("%c\n",ch);
}
