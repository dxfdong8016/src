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
    dispatch_fn fn;//�������Ĳ�����ȻΪ�����arg[128]
    char arg[128];
    threadpool parent;/*typedef void * threadpool ;*/
} _thread;

// _threadpool���ڲ��̳߳ؽṹ��ת�������͡�threadpool�����ύ��ʹ����֮ǰ
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

    int tp_total;//��ǰ�̳߳����������е��߳���
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

//����match����
void interface_match(void *myparam);

void app_match(void *myparam);

void all_match(void *myparam);

#define MAX_FILE_COUNT DETAILFILE_SAVETIME*24*60/DETAIL_SEPARATE_TIME

match_parameter file_names[MAX_FILE_COUNT];



//����߳����ڳ���
#define MAXT_IN_POOL 200

//Ϊ����ʹ���������̳߳ص��ڲ��ṹϸ�ڣ���threadpool����Ϊvoid*
//��threadpool.c�п���������ת��ת��������ϸ���뿴threadpool.c


//�����̶���С���̳߳أ���������ɹ����������طǿ�ֵ�����򷵻ؿ�ֵ
threadpool create_threadpool(int num_threads_in_pool);

//����һ���߳��������������������̶߳��ǿ��У����ȳ�������ֱ�����߳̿��в����ϵ���
//һ��һ���̱߳����ȣ��������Ϸ���
//����̵߳��ú���dispathch_to_here��arg��Ϊ��������
int dispatch_threadpool(threadpool from_me, dispatch_fn dispatch_to_here, char *arg);

//�����̳߳أ�ʹ���������߳���ɱ��֮���ͷ���������ڴ�
void destroy_threadpool(threadpool destroyme);

int pthread_pool_insert_file(int filecount,int interface_flag);

int get_file_count(struct tm *tm_start,struct tm *tm_end);

void get_detail_file_name(int filecount,match_parameter file[],struct tm *tm_start,struct tm *tm_end,match_st mymatch_param);

#ifdef __cplusplus
}
#endif

#endif
