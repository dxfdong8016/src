/*******************************************************************
* Copyright (C), 2009-2019, DongHua Software Co.,Ltd
* File name: interface.h

********************************************************************/
#ifndef __FLOWDETAIL_H__
#define __FLOWDETAIL_H__

#include "config.h"
#include "path.h"
#include "shmem.h"
#include <sys/stat.h>
#include <getopt.h>


#define IPV6_ISIPV4(x) ((0 == (x)[2]) && (0 == (x)[1]) && (0 == (x)[0]) ? 1 : 0)
#define IPV6_ISZERO(x) ((0 == (x)[3]) && (0 == (x)[2]) && (0 == (x)[1]) && (0 == (x)[0]) ? 1 : 0)
#define IPV6_NOZERO(x) ((0 != (x)[3]) || (0 != (x)[2]) || (0 != (x)[1]) || (0 != (x)[0]) ? 1 : 0)
#define IPV6_EQUAL(x,y) (((x)[3] == (y)[3]) && ((x)[2] == (y)[2]) && ((x)[1] == (y)[1]) && ((x)[0] == (y)[0]) ? 1 : 0)
#define IPV6_NOEQUAL(x,y) (((x)[3] != (y)[3]) || ((x)[2] != (y)[2]) || ((x)[1] != (y)[1]) || ((x)[0] != (y)[0]) ? 1 : 0)
	

/* use in write detail file*/
#define SQL_COLLECT "/data/sql"
/* topn file path */
#define TOPN_FILE_PATH "/data/detailtemp"
/* detail file path */
#define DETAIL_PATH "/data/detail"

#define IPALL_NOTOP_PATH "/data/ip_all"
#define INTERFACE_STATE_PATH "/dhcc/forceview/cfgbackup/interface"
#define MAX_ROUTER_COUNT 250  	//Max number of supported routers
#define MAX_IF_COUNT 200       	//Max number of supported interfaces per router

/* Router Interface application analyze hash size and total number */
#define APP_COUNT 1024             /* application hash block size */
#define APP_COUNT_DETAIL 128     /* application detail hash block size */
#define APP_COUNT_MAX 8192         /* application counter max number */
#define APP_COUNT_DETAIL_MAX 512   /* application detail counter max number */
#define APP_PORT_TOPN_INDB 50   //Max port number write into file per interface

#define IP_NUM  1024 
#define IP_DETAIL_NUM 256 
#define IP_TOTAL_NUM 2048  //每个接口下的内网或外网IP总个数
#define IP_DETAIL_TOTAL_NUM 256  //每个IP的细节总个数
#define IP_TOPN 50
#define IP_DETAIL_TOPN 50

#define ICMP  1       // Protocol ICMP
#define TCP  6        // Protocol TCP
#define UDP  17       // Protocol UDP

struct  app_ip_detail_t{
	u_int32 saddr[4];
	u_int32 daddr[4];
	u_int64 sd_octets;   //saddr->daddr
	u_int64 ds_octets;  //daddr->saddr
	u_int32 sd_pkts;
	u_int32 ds_pkts;
	u_int32 sd_flows;
	u_int32 ds_flows;
	struct  app_ip_detail_t * ip_next;
};

struct app_t{
	u_int8 rid;
	u_int8 proto;
	u_int32 index;
	u_int16 port;
	u_int8 portid;       	//1：port; 2：port group
	u_int8 detail_flag;     //0：no detail; 1：detail
	u_int16 detail_ip_num;  //detail IP num
	u_int16 port_num;
	u_int64 in_octets;   	//inif
	u_int64 out_octets; 	//outif
	u_int32 in_pkts;
	u_int32 out_pkts;
	u_int32 in_flows; 
	u_int32 out_flows;
	struct  app_t *port_next;  			//port LINK node num
	struct  app_ip_detail_t *ip_detail;  		//IP addr pair
};

struct app_port_topn_t{
	u_int8  rid;
	u_int8  portid;  //0：port; 1：port group
	u_int8 	proto;
	u_int32 index;
	u_int16 port;
	u_int64 octets;
};

struct time_set_t{
	char time_str[32];
	char tim[32];
	int week;
	int hour_file; //1: need to write hour file 0: need not write hour file
	char hour_time_str[32];
}time_set;

struct  ip_detail_t{
	u_int32   addr[4]; 
	u_int64   octets;
	u_int64   re_octets;
	u_int32   pkts;
	u_int32   re_pkts;
	u_int32   flows;
	u_int32   re_flows;
	u_int16   port;
	u_int8    protol;
	u_int8    port_flag;
	u_int32   detail_if;
	struct  ip_detail_t  *next_detail; 
};

struct  ip_t{
	u_int32   addr[4];    
	u_int64   in_octets; 
	u_int64   out_octets; 
	u_int32   in_pkts;  
	u_int32   out_pkts; 
	u_int32   in_flows;
	u_int32   out_flows;
	struct    ip_t	*next; 
	struct    ip_detail_t	**ip_detail;  
	u_int16   ip_detail_total_count;
	u_int8    detail_flag; 	//1:have detail ,  0:no detail
};

struct interface_ip{
  u_int32  ifindex; 
  u_int16  ip_innet_count;
  u_int16  ip_outnet_count;
  struct  ip_t   ip_in[IP_NUM];
  struct  ip_t   ip_out[IP_NUM];
};

struct ip_top{
	u_int32   ifindex; 
	u_int32   addr_topin[IP_TOPN][4];
	u_int32   addr_topout[IP_TOPN][4];
	struct    ip_t    *topin_point[IP_TOPN];
	struct    ip_t    *topout_point[IP_TOPN];
};

void ft_app_sort(struct flow pdu);
void ft_app_file();
void ft_app_clean();

void ft_ip_sort(struct flow pdu);
void ft_ip_topn(void); 
void ft_ip_detail_topn(struct ip_t *ipcur);
int ft_ip_indb(void);
void ft_ip_clean(void); 

extern int dir_ishaving(char *dir_name);
int version_info(void);
int write_log(char *log_msg);
int ft_get_topn(char *filename);
int ft_get_flow(char *filename);

static int ipv6_ntoa(const unsigned int *ipint, char* ipstr, int len);
static int ipv6_cmp(const unsigned int *ip1,const unsigned int *ip2);
#endif

