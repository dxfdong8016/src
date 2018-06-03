#ifndef __threadpool_h__
#define __threadpool_h__

#ifdef __cplusplus
extern "C" ...{
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "path.h"
#include "config.h"
#include "nfpdu.h"


//#define DETAIL_SEPARATE_TIME ROUTER_APP_DETAIL
#define DETAIL_SEPARATE_TIME 5
//#define DETAILFILE_SAVETIME MONITOR_APP_DETAIL
#define DETAILFILE_SAVETIME 7
#define DETAILFILEPATH  "/data/detail"
#define MATCHEDFILEPATH  "/data/detailtemp"



#define THREADPOOL_NUM  5

static  int hisDataSaveTime_m = 0;  
static  int hisDataSaveTime_h = 0;  
static  int hisDataSaveTime_d = 0; 
static  int allTrafficSaveTime = 0; 
static  int ROUTER_APP_DETAIL = 0;
static  int ROUTER_IP_DETAIL = 0;
static  int ROUTER_AS_DETAIL = 0;
static  int MONITOR_APP_DETAIL = 0; 
static  int MONITOR_IP_DETAIL = 0;
static  int MONITOR_AS_DETAIL = 0;
static  int HAS_IPPAIR = 0; 
static  int HAS_TOS = 0; 
static  int HAS_SUBNET = 0;
static  int HAS_ASPAIR = 0;
static  int HAS_AS = 0;
static  int HAS_PKT = 0;
static  int HAS_NEXTHOP = 0; 

static u_int32 interfaces[64];

char temp_dir[64];
char temp_ip[64];

typedef void (*dispatch_fn)(void *);
typedef void *threadpool;

typedef struct _thread_st{
    pthread_t id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    dispatch_fn fn;//本函数的参数居然为下面的arg[128]
    char arg[128];
    threadpool parent;/*typedef void * threadpool ;*/
} _thread;

// _threadpool是内部线程池结构，转换成类型“threadpool”在提交给使用者之前
typedef struct _threadpool_st {
    // you should fill in this structure with whatever you need
    pthread_mutex_t tp_mutex;
    pthread_cond_t tp_idle;
    pthread_cond_t tp_full;
    pthread_cond_t tp_empty;
    _thread ** tp_list;
    int tp_index;
    int tp_max_index;
    int tp_stop;

    int tp_total;//当前线程池中正在运行的线程数
} _threadpool;

typedef struct {
	u_int8  rid;
	u_int8  mid;
	u_int32 ip;
	u_int16 as;
	u_int16 gid;
	u_int32 myif;
	u_int16 port;
	u_int8  proto;
} match_st;

typedef struct{
	match_st mymatch;
	
	char absolutepath[128];
	char detailfile[32];
	
	char tempfilepath[128];
	char tempfile[32];
	
	char finalfile[32];
}match_parameter;

//两个match函数
void interface_match(void *myparam);

void app_match(void *myparam);

void all_match(void *myparam);

#define MAX_FILE_COUNT DETAILFILE_SAVETIME*24*60/DETAIL_SEPARATE_TIME

match_parameter file_names[MAX_FILE_COUNT];



//最大线程数在池中
#define MAXT_IN_POOL 200

//为了向使用者隐藏线程池的内部结构细节，将threadpool声明为void*
//在threadpool.c中可以用类型转换转换回来，细节请看threadpool.c


//创建固定大小的线程池，如果创建成功，函数返回非空值，否则返回空值
threadpool create_threadpool(int num_threads_in_pool);

//分配一个线程完成请求，如果池中所有线程都非空闲，调度程序阻塞直到有线程空闲并马上调度
//一旦一个线程被调度，函数马上返回
//这个线程调用函数dispathch_to_here，arg作为函数参数
int dispatch_threadpool(threadpool from_me, dispatch_fn dispatch_to_here, char *arg);

//销毁线程池，使池中所有线程自杀，之后释放所有相关内存
void destroy_threadpool(threadpool destroyme);

int pthread_pool_insert_file(int filecount,int interface_flag);

int get_file_count(struct tm *tm_start,struct tm *tm_end);

void get_detail_file_name(int filecount,match_parameter file[],struct tm *tm_start,struct tm *tm_end,match_st mymatch_param);

#ifdef __cplusplus
}
#endif

#endif
