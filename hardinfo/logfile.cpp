#include "cstdio"
#include "cstring"
#include "algorithm"
#include "cstdlib"
#include "ctime"
#include "unistd.h"
#define MAX_CMD_LENGTH 1000
#define MAX_STR_LENGTH 1000
using namespace std;
struct logfile{
	char filename[100];
	long long inode;
	int mtime;
	int md5size;
	long long size;
	long long processed_size;
	char md5buff;
}
