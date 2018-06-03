/*
*2014-12-26  add mv detail file to IPALL_NOTOP_PATH
*2015-09-02  change file name SQLDEL_ip* -> SQLDETAIL_ip* , SQLDEL_app* -> SQLDETAIL_app*
*2016-06-22  change ipv4 -> ipv6
*
*/
#include "flow-detail.h"

#define VERSION_V4 4 
#define VERSION_V6 6

static int router_id_record[MAX_ROUTER_COUNT];
static int max_rid_count;
static int rid_list[MAX_ROUTER_COUNT];

static struct app_t *app[MAX_ROUTER_COUNT][MAX_IF_COUNT];
static struct app_port_topn_t *app_port_topn[MAX_ROUTER_COUNT][MAX_IF_COUNT];

static struct interface_ip *ip_past[MAX_ROUTER_COUNT][MAX_IF_COUNT];
static struct ip_top  *iptop[MAX_ROUTER_COUNT][MAX_IF_COUNT];

static char detailfile_day[10];
static char detailfile_min[10];
static time_t version_file_time;
static int year;
static int mon;
static int day;
static int hour;
static int min;
static int TIME_INTERVAL;

static int TOTAL_TOPN, DETAIL_TOPN;
static int IP_ALL_FLAG;
static int HAS_IP_DETAIL, HAS_APP_DETAIL;  /*0:no detail, 1:has detail*/
static int if_total[MAX_ROUTER_COUNT];
static int topn_change_flag;
static int bzip_flag = 0;

static const char short_options[] = "s";
static const struct option long_options[] = {
	{"save", no_argument, NULL, 's'},
	{0, 0, 0, 0}
};


int main(int argc, char ** argv){
	daemon(0,0);
	for( ; ; ){
		int index;
		int c;
		c = getopt_long(argc, argv, short_options, long_options, &index);
		if( -1 == c){
			break;	
		}
		switch(c){
			case 0:
				break;
			case 's':
				bzip_flag = 1;
				break;
			default :
				printf("err\n");
				exit(0);
				break;
		}
	}
	
	int file_num = 0,max_if_num = 0 ,i = 0;
	DIR *dir;
	struct dirent *dp;
	
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(2, &mask);
	/* sched_setaffinity returns 0 in success */
	if (sched_setaffinity(0, sizeof(mask), &mask) == -1){
		write_log("sched_setaffinity() failed");
	}
	
	version_info();
	if (0 == TIME_INTERVAL || 0 == TOTAL_TOPN){
		write_log("read version.xml  failed, argument error");
		return -1;
	}
	dir_ishaving(IPALL_NOTOP_PATH);
	
	while(1){
		max_if_num = if_total[0];
		for(i = 1; i < MAX_ROUTER_COUNT; i++){
			if (0 == if_total[i]){
				continue;
			}
			if (max_if_num < if_total[i]){
				max_if_num = if_total[i];
			}
		}
		
		memset(rid_list,0,sizeof(rid_list));
		dir = opendir(TOPN_FILE_PATH);
		while ((dp = readdir(dir)) != NULL){
			file_num = 0;
			if (0 == strncmp(dp->d_name, "R",1)){
				file_num = 1;
				sscanf(dp->d_name,"%*[^_]_%[^_]_%[^_]",detailfile_day,detailfile_min);
			
				ft_get_topn(dp->d_name);
				ft_get_flow(dp->d_name);
				
				ft_app_file();   	
				ft_app_clean();  		
      		
				ft_ip_topn();
				ft_ip_indb();
				ft_ip_clean();		
			}
		}
		closedir(dir);
				
		if(0 == file_num){
			sleep(5);	
		}
		memset(if_total, 0, MAX_ROUTER_COUNT * sizeof(int));
		version_info();
	}
	return 0;
}

/********************************************************************
* Function: ft_get_topn   
* Description: get topN info
* Input: char*
* Output: 
* Return: int
* Others: 
********************************************************************/
int ft_get_topn(char *filename){
	unsigned int r_id = 0, if_index = 0, port = 0, portid = 0, proto = 0, inaddr[4] = {0,0,0,0}, outaddr[4] = {0,0,0,0};
	int i, j, app_flag = 0, ifindex_num = 0, apptopn_num = 0, ipin_flag = 0, ipout_flag = 0,ip_num = 0, iptopnin_num = 0, iptopnout_num = 0,mem_flag = 0;
	int is_firstapp = 0,last_ifindex = 0,last_rid = 0,is_firstip = 0,last_iprid = 0;
	FILE *fp = NULL;//*fp_test = NULL;
	char logfile[512];
	char filepath[512];
	char databuf[512]; 
	char command[512];
	
	for (i = 1; i <= max_rid_count; i++){
		for (j = 0; j < MAX_IF_COUNT; j++){
			if(NULL == app_port_topn[i][j] && NULL == iptop[i][j]){
				break;
			}
			if (app_port_topn[i][j] != NULL){
				free(app_port_topn[i][j]);
				app_port_topn[i][j] = NULL;
			}
			if (iptop[i][j] != NULL){
				free(iptop[i][j]);
				iptop[i][j] = NULL; 
			}
		}
	}
	max_rid_count = 0;
	memset(rid_list,0,sizeof(rid_list));	
	sprintf(filepath,"%s/%s",TOPN_FILE_PATH,filename);
	if(NULL == (fp = fopen(filepath,"r"))){
		sprintf(logfile,"open file %s failed",filename);
		write_log(logfile);
		return -1;
	}
	
	while(fgets(databuf,512,fp) != NULL){
		if(0 == app_flag && strstr(databuf,"app") != NULL){
			app_flag = 1;
			continue;
		}
		if(0 == ipin_flag && strstr(databuf,"ipin") != NULL){
			ipin_flag = 1;
			ipout_flag = 0;
			app_flag = 0;
			iptopnin_num = 0;
			mem_flag = 0;
			if(is_firstip != 0 && if_index != 0){
				ip_num++;	
			}
			continue;
		}
		if(0 == ipout_flag && strstr(databuf,"ipout") != NULL){
			ipout_flag = 1;
			ipin_flag = 0;
			iptopnout_num = 0;
			continue;
		}
		if(1 == app_flag && 0 == ipin_flag && 0 == ipout_flag){
			sscanf(databuf,"%u|%u|%u|%u|%u",&r_id,&if_index,&port,&portid,&proto);
			if(0 == r_id || 0 == if_index){
				continue;
			}
			if(r_id > max_rid_count){
				max_rid_count = r_id;
			}
			if(0 == is_firstapp){
				is_firstapp = 1;
				last_ifindex = if_index;
				last_rid = r_id;
				
				app_port_topn[r_id][ifindex_num] = (struct app_port_topn_t *)malloc(TOTAL_TOPN * sizeof(struct app_port_topn_t));
				if(NULL == app_port_topn[r_id][ifindex_num]){
					write_log("app_topn malloc mem failed");
					return -1;
				}
				memset(app_port_topn[r_id][ifindex_num],0,TOTAL_TOPN * sizeof(struct app_port_topn_t));
				
				(app_port_topn[r_id][ifindex_num]+apptopn_num)->rid 		= r_id;
				(app_port_topn[r_id][ifindex_num]+apptopn_num)->index 	= if_index;
				(app_port_topn[r_id][ifindex_num]+apptopn_num)->port 		= port;
				(app_port_topn[r_id][ifindex_num]+apptopn_num)->portid	= portid;
				(app_port_topn[r_id][ifindex_num]+apptopn_num)->proto 	= proto;
				(app_port_topn[r_id][ifindex_num]+apptopn_num)->octets 	= 1;
				apptopn_num++;
			}
			else if(is_firstapp != 0){
				if(last_ifindex == if_index && last_rid == r_id){
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->rid 		= r_id;
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->index 	= if_index;
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->port 		= port;
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->portid	= portid;
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->proto 	= proto;
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->octets 	= 1;
					apptopn_num++;
				}
				else{
					if(last_rid == r_id && last_ifindex != if_index){
						ifindex_num++;
						apptopn_num = 0;
					}
					else if(last_rid != r_id){
						ifindex_num = 0;
						apptopn_num = 0;		
					}
					last_rid = r_id;
					last_ifindex = if_index;
					
					app_port_topn[r_id][ifindex_num] = (struct app_port_topn_t *)malloc(TOTAL_TOPN * sizeof(struct app_port_topn_t));
					if(NULL == app_port_topn[r_id][ifindex_num]){
						write_log("app_topn malloc mem failed");
					}
					memset(app_port_topn[r_id][ifindex_num],0,TOTAL_TOPN * sizeof(struct app_port_topn_t));
					
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->rid 		= r_id;
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->index 	= if_index;
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->port 		= port;
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->portid	= portid;
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->proto 	= proto;
					(app_port_topn[r_id][ifindex_num]+apptopn_num)->octets 	= 1;
					apptopn_num++;
				}
			}
		}
		else if(1 == ipin_flag && 0 == ipout_flag){
			sscanf(databuf,"%u|%u|%u|%u|%u|%u",&r_id,&if_index,&inaddr[0],&inaddr[1],&inaddr[2],&inaddr[3]);
			if(0 == r_id || 0 == if_index){
				continue;
			}
			rid_list[r_id] = r_id;
			if(r_id > max_rid_count){
				max_rid_count = r_id;
			}
			
			if(0 == is_firstip){
				is_firstip = 1;
				last_iprid = r_id;
			  mem_flag = 1;

				iptop[r_id][ip_num] = (struct ip_top *)malloc(sizeof(struct ip_top));		
				if (NULL == iptop[r_id][ip_num]){
					write_log("iptop malloc mem failed");
					return -1;
				}
				memset(iptop[r_id][ip_num], 0, sizeof(struct ip_top));
				iptop[r_id][ip_num]->ifindex = if_index;
				iptop[r_id][ip_num]->addr_topin[iptopnin_num][0] = inaddr[0];
				iptop[r_id][ip_num]->addr_topin[iptopnin_num][1] = inaddr[1];
				iptop[r_id][ip_num]->addr_topin[iptopnin_num][2] = inaddr[2];
				iptop[r_id][ip_num]->addr_topin[iptopnin_num][3] = inaddr[3];
				iptopnin_num++;
			}
			else if(is_firstip != 0){
				if(0 == mem_flag){
					mem_flag = 1;
					if(last_iprid != r_id){
						ip_num = 0;	
					}
					iptop[r_id][ip_num] = (struct ip_top *)malloc(sizeof(struct ip_top));
					if (NULL == iptop[r_id][ip_num]){
						write_log("iptop malloc mem failed");
						return -1;
					}
					memset(iptop[r_id][ip_num], 0, sizeof(struct ip_top));
					iptop[r_id][ip_num]->ifindex = if_index;
				}
				last_iprid = r_id;
				iptop[r_id][ip_num]->addr_topin[iptopnin_num][0] = inaddr[0];
				iptop[r_id][ip_num]->addr_topin[iptopnin_num][1] = inaddr[1];
				iptop[r_id][ip_num]->addr_topin[iptopnin_num][2] = inaddr[2];
				iptop[r_id][ip_num]->addr_topin[iptopnin_num][3] = inaddr[3];
				iptopnin_num++;
			}
		}
		else if(1 == ipout_flag && 0 == ipin_flag){
			sscanf(databuf,"%u|%u|%u|%u|%u|%u",&r_id,&if_index,&outaddr[0],&outaddr[1],&outaddr[2],&outaddr[3]);

			rid_list[r_id] = r_id;
			if(0 == r_id || 0 == if_index){
				continue;
			}
			if(r_id > max_rid_count){
				max_rid_count = r_id;
			}
				
			last_iprid = r_id;
			iptop[r_id][ip_num]->ifindex = if_index;
			iptop[r_id][ip_num]->addr_topout[iptopnout_num][0] = outaddr[0];
			iptop[r_id][ip_num]->addr_topout[iptopnout_num][1] = outaddr[1];
			iptop[r_id][ip_num]->addr_topout[iptopnout_num][2] = outaddr[2];
			iptop[r_id][ip_num]->addr_topout[iptopnout_num][3] = outaddr[3];
			iptopnout_num++;
		}
		memset(databuf,0,512);
	}
	fclose(fp);
	sprintf(command,"/bin/rm -f %s",filepath);
	system(command);
	return 0;
}

/********************************************************************
* Function: ft_get_flow   
* Description: get flow from file
* Input: char*
* Output: 
* Return: int
* Others: 
********************************************************************/
int ft_get_flow(char *filename){
	int i = 0;
	FILE *fp;
	int first_line = 0;
	char detailfile[128];
	char logfile[128];
	char databuf[512];
	char command[128];
	struct flow pdu;

	sscanf(filename,"%*[^_]_%[^_]_%[^_]_%d",detailfile_day,detailfile_min,&time_set.week);
	sscanf(detailfile_day,"%4d",&year);
	sscanf(&detailfile_day[4],"%2d",&mon);
	sscanf(&detailfile_day[6],"%2d",&day);
	sscanf(detailfile_min,"%2d",&hour);
	sscanf(&detailfile_min[2],"%2d",&min);
	
	sprintf(time_set.time_str,"%04d-%02d-%02d %02d:%02d:00",year,mon,day,hour,min);
	sprintf(time_set.tim,"%04d%02d%02d%02d%02d00",year,mon,day,hour,min);
	
	for(i = 1; i <= max_rid_count; i++){
		if(0 == rid_list[i]){
			continue;
		}
		
		sprintf(detailfile,"%s/%s/R%d/%s",DETAIL_PATH,detailfile_day,i,detailfile_min);
		if((fp = fopen(detailfile,"r")) != NULL){
			first_line = 0;
			while(fgets(databuf,sizeof(databuf),fp) != NULL){
				memset(&pdu,0,sizeof(pdu));
				if(0 == first_line){
					first_line = 1;
					continue;
				}
				sscanf(databuf,"%hhu|%u|%u|%hu|%hhu|%hhu|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u",&pdu.rid,&pdu.inif,&pdu.outif,&pdu.port,&pdu.portid,&pdu.proto,&pdu.saddr[0],&pdu.saddr[1],&pdu.saddr[2],&pdu.saddr[3],&pdu.daddr[0],&pdu.daddr[1],&pdu.daddr[2],&pdu.daddr[3],&pdu.pkts,&pdu.octets);
				if(0 == rid_list[pdu.rid]){
					continue;
				}
				ft_app_sort(pdu);
				ft_ip_sort(pdu);
			}
			fclose(fp);
			if(bzip_flag != 0){
				if(IP_ALL_FLAG != 1){
					sprintf(command,"/usr/bin/bzip2 --repetitive-fast %s > /dev/null 2>&1",detailfile);
				}
				else{
					sprintf(command,"/bin/mv %s %s/%s%s_%d > /dev/null 2>&1",detailfile,IPALL_NOTOP_PATH,detailfile_day,detailfile_min,pdu.rid);
				}
				system(command);
			}
			else{
				sprintf(command,"/bin/rm -f %s",detailfile);
				system(command);
			}
		}
		else{	
			sprintf(logfile,"open %s failed",detailfile);
			write_log(logfile);
		}
	}
	return 0;
}

/********************************************************************
* Function: ft_app_sort   
* Description: Analysis application(APP) data£¬save data into the struct array
* Input: struct flow pdu
* Output: 
* Return: void
* Others: 
********************************************************************/
void ft_app_sort(struct flow pdu){
	int i, j, k; 			//Count variable
	u_int16 count;  	//count: postion of router
	int pos; 				//HASH postion 
	struct app_t* app_temp;  		//Point to temp PORT node
	struct app_t* app_new;   		//Point tonew PORT node
	
	struct  app_ip_detail_t *ip_detail_new_array;  //Point to new DETAIL IP array
	struct  app_ip_detail_t *ip_detail_new_node;   //Point to new DETAIL IP node
	struct  app_ip_detail_t *ip_detail_temp_node;  //Point to temp DETAIL IP node
	
	app_temp = NULL;
	app_new = NULL;
	ip_detail_new_array = NULL;
	ip_detail_new_node = NULL;
	ip_detail_temp_node = NULL;
	
	count = pdu.rid;  //position  of router  in  router's config file
	
	//save router_id
	router_id_record[pdu.rid] = pdu.rid;
	
	/*inif*/
	for (i = 0; i < MAX_IF_COUNT; i++){
		if(0 == pdu.inif){
			break;
		}
		if (pdu.proto != TCP && pdu.proto != UDP){
			break;
		}
		
		if(NULL == app[count][i]){
			app[count][i] = (struct app_t *)malloc(APP_COUNT * sizeof(struct app_t));
			if(NULL == app[count][i]){
				write_log("malloc app failed\n");
				return;
			}
			if_total[count]++;
			memset(app[count][i], 0, APP_COUNT * sizeof(struct app_t));
			
			//record if_index  in position 0	
			(app[count][i]+0)->index = pdu.inif;
			
			/* Record top APP_PORT_TOPN_INDB  port per interface into app array 
			* if a port is recorded, it can save detail IP address */
			for (j = 0; j < MAX_IF_COUNT; j++){
				if(0 == HAS_APP_DETAIL){  //no detail
					break;
				}
				if(NULL == app_port_topn[count][j]){
					break;    
				}
				else if((app_port_topn[count][j]+ 0)->index == pdu.inif){
					for (k = 0; k < TOTAL_TOPN; k++){
						if(0 == (app_port_topn[count][j]+ k)->octets){
							break;
						}
						pos = ((app_port_topn[count][j]+ k)->port) % APP_COUNT;
						
						//Position POS have not been recorded
						if (0 == (app[count][i] + pos)->detail_flag){
							(app[count][i] + pos)->port 				= (app_port_topn[count][j]+ k)->port;
							(app[count][i] + pos)->portid 			= (app_port_topn[count][j]+ k)->portid;
							(app[count][i] + pos)->proto 				= (app_port_topn[count][j]+ k)->proto;
							(app[count][i] + pos)->detail_flag 	= 1;
							(app[count][i] + pos)->port_next 		= NULL;
							(app[count][i] + pos)->ip_detail 		= NULL;	  
						}
						else if (1 == (app[count][i] + pos)->detail_flag){
							app_temp = app[count][i] + pos;
							
							while(app_temp->port_next != NULL){
								app_temp = app_temp->port_next;
							}
							
							// the final  port node
							if (NULL == app_temp->port_next){
								app_new = (struct app_t*)malloc(sizeof(struct app_t));
								if (NULL == app_new){
									return;
								}
								memset(app_new, 0, sizeof(struct app_t));
								  
								app_new->port 				= (app_port_topn[count][j]+ k)->port;
								app_new->portid 			= (app_port_topn[count][j]+ k)->portid;
								app_new->proto 				= (app_port_topn[count][j]+ k)->proto;
								app_new->detail_flag 	= 1;
								app_new->port_next 		= NULL;
								app_new->ip_detail 		= NULL;	 
	
								app_temp->port_next 	= app_new; 
								app_new 							= NULL;
							}
						}
					}
					break;
				}
			}
			
			pos = pdu.port % APP_COUNT;
			app_temp = app[count][i] + pos;
			
			if (0 == app_temp->detail_flag){
				break;
			}
			else if (1 == app_temp->detail_flag){
				while(1){
					if ((app_temp->port == pdu.port)&&(app_temp->portid == pdu.portid)&&(app_temp->proto == pdu.proto)){
						app_temp->index = pdu.inif;
						
						ip_detail_new_array =  (struct  app_ip_detail_t *)malloc((APP_COUNT_DETAIL) * sizeof(struct  app_ip_detail_t));
						if (NULL == ip_detail_new_array){
							return;
						}
						memset(ip_detail_new_array, 0, APP_COUNT_DETAIL * sizeof(struct  app_ip_detail_t));
						  
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[0] 	= pdu.saddr[0];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[1] 	= pdu.saddr[1];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[2] 	= pdu.saddr[2];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[3] 	= pdu.saddr[3];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[0] 	= pdu.daddr[0];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[1] 	= pdu.daddr[1];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[2] 	= pdu.daddr[2];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[3] 	= pdu.daddr[3];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->sd_octets = pdu.octets;
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->sd_pkts 	= pdu.pkts;
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->sd_flows 	= 1;
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->ip_next 	= NULL;
							
						app_temp->ip_detail = ip_detail_new_array;	
						app_temp->detail_ip_num = 1;			 
							
						ip_detail_new_array = NULL; 
						break;
					}
					else if(NULL == app_temp->port_next){
						break;
					}
					app_temp = app_temp->port_next;
				}
			}
			break;
		}
		else if ((app[count][i] + 0)->index == pdu.inif){
			pos = pdu.port % APP_COUNT;
			app_temp = app[count][i] + pos;
			if (0 == app_temp->detail_flag){
					break;
			}
			else{
				while(1){
					if((app_temp->port == pdu.port)&&(app_temp->portid == pdu.portid)&&(app_temp->proto == pdu.proto)){
						app_temp->index = pdu.inif;
						if(NULL == app_temp->ip_detail){
							ip_detail_new_array =  (struct  app_ip_detail_t *)malloc(APP_COUNT_DETAIL * sizeof(struct  app_ip_detail_t));
							if(NULL == ip_detail_new_array){
								return;
							}
							memset(ip_detail_new_array, 0, APP_COUNT_DETAIL * sizeof(struct  app_ip_detail_t));
								  
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[0] 	= pdu.saddr[0];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[1] 	= pdu.saddr[1];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[2] 	= pdu.saddr[2];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[3] 	= pdu.saddr[3];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[0] 	= pdu.daddr[0];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[1] 	= pdu.daddr[1];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[2] 	= pdu.daddr[2];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[3] 	= pdu.daddr[3];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->sd_octets = pdu.octets;
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->sd_pkts 	= pdu.pkts;
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->sd_flows 	= 1;
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->ip_next 	= NULL;
								
							app_temp->ip_detail = ip_detail_new_array;	
							app_temp->detail_ip_num = 1;			 
							ip_detail_new_array = NULL;
						}
						else if (app_temp->ip_detail != NULL){
							ip_detail_temp_node = app_temp->ip_detail + (pdu.saddr[3]&pdu.daddr[3]) % APP_COUNT_DETAIL;
							
							while(1){
								if (IPV6_EQUAL(ip_detail_temp_node->saddr, pdu.saddr) && IPV6_EQUAL(ip_detail_temp_node->daddr, pdu.daddr)){
									ip_detail_temp_node->sd_octets 	+= pdu.octets;
									ip_detail_temp_node->sd_pkts 		+= pdu.pkts;
									ip_detail_temp_node->sd_flows++;
									break;
								}
								else if (IPV6_EQUAL(ip_detail_temp_node->saddr, pdu.daddr) && IPV6_EQUAL(ip_detail_temp_node->daddr, pdu.saddr)){
									ip_detail_temp_node->ds_octets += pdu.octets;
									ip_detail_temp_node->ds_pkts += pdu.pkts;
									ip_detail_temp_node->ds_flows++;
									break;
								}
								else if (ip_detail_temp_node->ip_next == NULL){
									//not beyond max detail IP number
									if (app_temp->detail_ip_num >= APP_COUNT_DETAIL_MAX){
										break;
									}
									
									ip_detail_new_node = (struct  app_ip_detail_t *)malloc(sizeof(struct  app_ip_detail_t));
									if(NULL == ip_detail_new_node){
										return;
									}
									memset(ip_detail_new_node, 0,  sizeof(struct  app_ip_detail_t));
									  
									ip_detail_new_node->saddr[0] 	= pdu.saddr[0];
									ip_detail_new_node->saddr[1] 	= pdu.saddr[1];
									ip_detail_new_node->saddr[2] 	= pdu.saddr[2];
									ip_detail_new_node->saddr[3] 	= pdu.saddr[3];
									ip_detail_new_node->daddr[0] 	= pdu.daddr[0];
									ip_detail_new_node->daddr[1] 	= pdu.daddr[1];
									ip_detail_new_node->daddr[2] 	= pdu.daddr[2];
									ip_detail_new_node->daddr[3] 	= pdu.daddr[3];
									ip_detail_new_node->sd_octets = pdu.octets;
									ip_detail_new_node->sd_pkts 	= pdu.pkts;
									ip_detail_new_node->sd_flows 	= 1;
									ip_detail_new_node->ip_next 	= NULL;
									app_temp->detail_ip_num++;  //detail IP num  current PORT			
									
									ip_detail_temp_node->ip_next = ip_detail_new_node;
									ip_detail_new_node = NULL;
									break;
								}
								ip_detail_temp_node = ip_detail_temp_node->ip_next;
							}
						}
						break;
					}
					else if(NULL == app_temp->port_next){
						break;
					}
					app_temp = app_temp->port_next;
				}
			}
			break;
		}
	}
	
	/*outif*/
	for (i = 0; i < MAX_IF_COUNT; i++){
		if(0 == pdu.outif){
			break;
		}
		if(pdu.proto != TCP && pdu.proto != UDP){
			break;
		}
		if(NULL == app[count][i]){
			app[count][i] = (struct app_t *)malloc(APP_COUNT * sizeof(struct app_t));
			if(NULL == app[count][i]){
				//printf("malloc app[%d][%d] failed\n",count,i);
				return;
			}
	
			if_total[count]++;
			memset(app[count][i], 0, APP_COUNT * sizeof(struct app_t));
		
			//record if_index  in position 0	
			(app[count][i] + 0)->index = pdu.outif;
			
			/* Record top APP_PORT_TOPN_INDB  port per interface into app array 
			* if a port is recorded, it can save detail IP address */
			for (j = 0; j < MAX_IF_COUNT; j++){
				if (0 == HAS_APP_DETAIL){  //no detail
					break;
				}
				
				if (NULL == app_port_topn[count][j]){
					break;    
				}
				else if( (app_port_topn[count][j]+ 0)->index == pdu.outif){
					for (k = 0; k < TOTAL_TOPN; k++){
						if(0 == (app_port_topn[count][j]+ k)->octets){
							break;
						}
						pos = ((app_port_topn[count][j]+ k)->port) % APP_COUNT;
						if (0 == (app[count][i] + pos)->detail_flag){
							(app[count][i] + pos)->port = (app_port_topn[count][j]+ k)->port;
							(app[count][i] + pos)->portid = (app_port_topn[count][j]+ k)->portid;
							(app[count][i] + pos)->proto = (app_port_topn[count][j]+ k)->proto;
							(app[count][i] + pos)->detail_flag = 1;
			
							(app[count][i] + pos)->port_next = NULL;
							(app[count][i] + pos)->ip_detail = NULL;
						}
						else if ((app[count][i] + pos)->detail_flag == 1){
							app_temp = app[count][i] + pos;
							
							while(app_temp->port_next != NULL){
								app_temp = app_temp->port_next;
							}
							
							// the final  port node
							if(NULL == app_temp->port_next){
								app_new = (struct app_t*)malloc(sizeof(struct app_t));
								if(NULL == app_new){
									return;
								}
								memset(app_new, 0, sizeof(struct app_t));
								  
								app_new->port = (app_port_topn[count][j]+ k)->port;
								app_new->portid = (app_port_topn[count][j]+ k)->portid;
								app_new->proto = (app_port_topn[count][j]+ k)->proto;
								app_new->detail_flag = 1;
								app_new->port_next = NULL;
								app_new->ip_detail = NULL;	
								 
								app_temp->port_next = app_new; 
								app_new = NULL;
							}
						}
					}
					break;
				}
			}
			
			pos = pdu.port % APP_COUNT;
			app_temp = app[count][i] + pos;
			
			if(0 == app_temp->detail_flag){
				break;
			}
			else if(1 == app_temp->detail_flag){
				while (1){
					if ((app_temp->port == pdu.port)&&(app_temp->portid == pdu.portid)&&(app_temp->proto == pdu.proto)){
						app_temp->index = pdu.outif;

						ip_detail_new_array =  (struct  app_ip_detail_t *)malloc((APP_COUNT_DETAIL) * sizeof(struct  app_ip_detail_t));
						if(NULL == ip_detail_new_array){
								return;
						}
						memset(ip_detail_new_array, 0, APP_COUNT_DETAIL * sizeof(struct  app_ip_detail_t));
							  
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[0] 	= pdu.saddr[0];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[1] 	= pdu.saddr[1];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[2] 	= pdu.saddr[2];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[3] 	= pdu.saddr[3];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[0] 	= pdu.daddr[0];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[1] 	= pdu.daddr[1];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[2] 	= pdu.daddr[2];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[3] 	= pdu.daddr[3];
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->ds_octets = pdu.octets;
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->ds_pkts 	= pdu.pkts;
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->ds_flows 	= 1;
						(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->ip_next 	= NULL;
							
						app_temp->ip_detail = ip_detail_new_array;	
						app_temp->detail_ip_num = 1;			 
							
						ip_detail_new_array = NULL; 		  	
						break;
					}
					else if(NULL == app_temp->port_next){
						break;
					}
					app_temp = app_temp->port_next;
				}
			}
			break;
		}
		else if((app[count][i] + 0)->index == pdu.outif){
			pos = pdu.port % APP_COUNT;
			app_temp = app[count][i] + pos;
			
			if(0 == app_temp->detail_flag){
					break;
			}
			else{
				while(1){
					if((app_temp->port == pdu.port)&&(app_temp->portid == pdu.portid)&&(app_temp->proto == pdu.proto)){
						app_temp->index = pdu.outif;
						if(NULL == app_temp->ip_detail){
							ip_detail_new_array =  (struct  app_ip_detail_t *)malloc(APP_COUNT_DETAIL * sizeof(struct  app_ip_detail_t));
							if(NULL == ip_detail_new_array){
								return;
							}
							memset(ip_detail_new_array, 0, APP_COUNT_DETAIL * sizeof(struct  app_ip_detail_t));
								  
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[0] 	= pdu.saddr[0];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[1] 	= pdu.saddr[1];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[2] 	= pdu.saddr[2];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->daddr[3] 	= pdu.saddr[3];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[0] 	= pdu.daddr[0];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[1] 	= pdu.daddr[1];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[2] 	= pdu.daddr[2];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->saddr[3] 	= pdu.daddr[3];
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->ds_octets = pdu.octets;
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->ds_pkts 	= pdu.pkts;
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->ds_flows 	= 1;
							(ip_detail_new_array + ((pdu.saddr[3]&pdu.daddr[3])%APP_COUNT_DETAIL))->ip_next 	= NULL;
								
							app_temp->ip_detail = ip_detail_new_array;	
							app_temp->detail_ip_num = 1;			 
								
							ip_detail_new_array = NULL;
						}
						else if(app_temp->ip_detail != NULL){
							ip_detail_temp_node = app_temp->ip_detail + (pdu.saddr[3]&pdu.daddr[3]) % APP_COUNT_DETAIL;
							while(1){
								if(IPV6_EQUAL(ip_detail_temp_node->saddr, pdu.saddr) && IPV6_EQUAL(ip_detail_temp_node->daddr, pdu.daddr)){
									ip_detail_temp_node->sd_octets += pdu.octets;
									ip_detail_temp_node->sd_pkts += pdu.pkts;
									ip_detail_temp_node->sd_flows++;
									break;
								}
								else if(IPV6_EQUAL(ip_detail_temp_node->saddr, pdu.daddr) && IPV6_EQUAL(ip_detail_temp_node->daddr, pdu.saddr)){
									ip_detail_temp_node->ds_octets += pdu.octets;
									ip_detail_temp_node->ds_pkts += pdu.pkts;
									ip_detail_temp_node->ds_flows++;
									break;
								}
								else if(NULL == ip_detail_temp_node->ip_next){
									if (app_temp->detail_ip_num >= APP_COUNT_DETAIL_MAX){
										break;
									}
										
									ip_detail_new_node = (struct  app_ip_detail_t *)malloc(sizeof(struct  app_ip_detail_t));
									if(NULL == ip_detail_new_node){
										return;
									}
									memset(ip_detail_new_node, 0,  sizeof(struct  app_ip_detail_t));
									ip_detail_new_node->daddr[0] = pdu.saddr[0];
									ip_detail_new_node->daddr[1] = pdu.saddr[1];
									ip_detail_new_node->daddr[2] = pdu.saddr[2];
									ip_detail_new_node->daddr[3] = pdu.saddr[3];
									ip_detail_new_node->saddr[0] = pdu.daddr[0];
									ip_detail_new_node->saddr[1] = pdu.daddr[1];
									ip_detail_new_node->saddr[2] = pdu.daddr[2];
									ip_detail_new_node->saddr[3] = pdu.daddr[3];
									ip_detail_new_node->ds_octets = pdu.octets;
									ip_detail_new_node->ds_pkts = pdu.pkts;
									ip_detail_new_node->ds_flows = 1;
									ip_detail_new_node->ip_next = NULL;
									app_temp->detail_ip_num++;  //detail IP num  current PORT			
					
									ip_detail_temp_node->ip_next = ip_detail_new_node;
									ip_detail_new_node = NULL;
									break;
								}
								ip_detail_temp_node = ip_detail_temp_node->ip_next;
							}
						}
						break;
					}
					else if(NULL == app_temp->port_next){
						break;
					}
					app_temp = app_temp->port_next;
				}
			}
			break;
		}
	}
}

/********************************************************************
* Function: ft_app_file   
* Description: Write application  analysis data of the struct array into file 
* Input: void
* Output: 
* Return: void
* Others: 
********************************************************************/
void ft_app_file(){
	int i, j, k, m, n, port, pos,router_id, file_stat = 0, indel_flag,index_num;
	FILE *f2;
	char  filename2[64], sip[64], dip[64], cmdline[128];
	
	struct app_t *app_temp = NULL;
	struct app_ip_detail_t app_ip_detail_max;
	struct app_ip_detail_t *app_ip_detail_temp = NULL;
	
	int v_flag_s, v_flag_s_old;
	int v_flag_d, v_flag_d_old;

	sprintf(filename2, "%s/appdel_tmp.%s", TMPDATAPATH, time_set.tim);
	f2 = fopen(filename2, "a+");
	if(NULL == f2){
		write_log("ft_app_file() failed to open appdel_tmp");
		return;
	}

	fprintf(f2, "use forceviewfa;\n");
	fprintf(f2,"set autocommit=0;\n");
	
	//write date
	for(i = 1; i <= max_rid_count; i++){
		router_id = router_id_record[i];
		
		for(j = 0; j < MAX_IF_COUNT; j++){
			if(NULL == app[i][j]){
				break;
			}
			for(index_num = 0; index_num < MAX_IF_COUNT;index_num++){
				if(NULL == app_port_topn[i][index_num]){
					break;
				}
				if((app_port_topn[i][index_num] + 0)->index == app[i][j]->index){
					file_stat = 1;
					for (k = 0; k < TOTAL_TOPN; k++){
						if(0 == (app_port_topn[i][index_num]+k)->octets){
							break;
						}
						else if ((app_port_topn[i][index_num]+k)->octets > 0){
							port =  (app_port_topn[i][index_num]+k)->port;
							pos = port % APP_COUNT;
							if(NULL == app[i][j]){
								break;
							}
							app_temp = app[i][j] + pos;
							//find PORT
							while ((app_temp->port != port)||(app_temp->portid != (app_port_topn[i][index_num]+k)->portid)||(app_temp->proto != (app_port_topn[i][index_num]+k)->proto)){   //use || or &&
								if(NULL == app_temp->port_next){
									break;
								}
								app_temp = app_temp->port_next;
							}
							
							//TOPN detail IP and write into file
							if ((app_temp->port == port)&&(app_temp->portid == (app_port_topn[i][index_num]+k)->portid)&&(app_temp->proto == (app_port_topn[i][index_num]+k)->proto)){
								if(1 == app_temp->detail_flag){
									if(NULL == app_temp->ip_detail){
										continue;
									}
									indel_flag = 0;
									
									for (m = 0; m < DETAIL_TOPN; m++){
										n = 0;
										app_ip_detail_temp = app_temp->ip_detail + n;
										memset(&app_ip_detail_max, 0, sizeof(struct app_ip_detail_t));
									
										while(!(NULL == app_ip_detail_temp->ip_next && n >= APP_COUNT_DETAIL)){
											if (app_ip_detail_temp->sd_octets + app_ip_detail_temp->ds_octets > app_ip_detail_max.sd_octets + app_ip_detail_max.ds_octets){
												app_ip_detail_max.saddr[0] = app_ip_detail_temp->saddr[0];
												app_ip_detail_max.saddr[1] = app_ip_detail_temp->saddr[1];
												app_ip_detail_max.saddr[2] = app_ip_detail_temp->saddr[2];
												app_ip_detail_max.saddr[3] = app_ip_detail_temp->saddr[3];
												app_ip_detail_max.daddr[0] = app_ip_detail_temp->daddr[0];
												app_ip_detail_max.daddr[1] = app_ip_detail_temp->daddr[1];
												app_ip_detail_max.daddr[2] = app_ip_detail_temp->daddr[2];
												app_ip_detail_max.daddr[3] = app_ip_detail_temp->daddr[3];
												app_ip_detail_max.sd_octets = app_ip_detail_temp->sd_octets;
												app_ip_detail_max.ds_octets = app_ip_detail_temp->ds_octets;
												app_ip_detail_max.sd_pkts = app_ip_detail_temp->sd_pkts;
												app_ip_detail_max.ds_pkts = app_ip_detail_temp->ds_pkts;
												app_ip_detail_max.sd_flows = app_ip_detail_temp->sd_flows;
												app_ip_detail_max.ds_flows = app_ip_detail_temp->ds_flows;
												app_ip_detail_max.ip_next = app_ip_detail_temp;
											}	
									
											if (NULL == app_ip_detail_temp->ip_next){
												if (n == APP_COUNT_DETAIL - 1){
													break;
												}
												
												n++;
												app_ip_detail_temp = app_temp->ip_detail+n;
											}
											else if (app_ip_detail_temp->ip_next != NULL){
												app_ip_detail_temp = app_ip_detail_temp->ip_next;
											}
										}
										
										if (0 == (app_ip_detail_max.sd_octets + app_ip_detail_max.ds_octets)){
											break;
										}
										else if (app_ip_detail_max.sd_octets + app_ip_detail_max.ds_octets > 0){
											v_flag_s = ipv6_ntoa(app_ip_detail_max.saddr, sip, 64);
											v_flag_d = ipv6_ntoa(app_ip_detail_max.daddr, dip, 64);
											
											if(0 == indel_flag){
												v_flag_s_old = v_flag_s;
												v_flag_d_old = v_flag_d;
												if(-1 == ipv6_cmp(app_ip_detail_max.saddr, app_ip_detail_max.daddr)){
													if(VERSION_V6 == v_flag_s || VERSION_V6 == v_flag_d){
														fprintf(f2, "insert into r%d_app_detail_%d(r_id,if_index,port,port_flag,proto_id,src_ip,dst_ip,\
															src_ip_int0,src_ip_int1,src_ip_int2,src_ip_int,dst_ip_int0,dst_ip_int1,dst_ip_int2,dst_ip_int,\
															octets,pkts,flows,re_octets,re_pkts,re_flows,col_time) values(%d, %u, %u, %u, %u, '%s','%s',\
															%u,%u,%u,%u,%u,%u,%u,%u,%llu,%u,%u,%llu,%u,%u,'%s')\n",router_id, time_set.week,router_id,\
															app_temp->index, app_temp->port, app_temp->portid, app_temp->proto, sip, dip, \
															app_ip_detail_max.saddr[0],app_ip_detail_max.saddr[1],app_ip_detail_max.saddr[2],app_ip_detail_max.saddr[3], \
															app_ip_detail_max.daddr[0],app_ip_detail_max.daddr[1],app_ip_detail_max.daddr[2],app_ip_detail_max.daddr[3], \
															app_ip_detail_max.sd_octets, app_ip_detail_max.sd_pkts, app_ip_detail_max.sd_flows,\
															app_ip_detail_max.ds_octets, app_ip_detail_max.ds_pkts, app_ip_detail_max.ds_flows, time_set.time_str);
													}
													else if(VERSION_V4 == v_flag_s && VERSION_V4 == v_flag_d){
														fprintf(f2, "insert into r%d_app_detail_%d(r_id,if_index,port,port_flag,proto_id,src_ip,dst_ip,\
															src_ip_int,dst_ip_int,\
															octets,pkts,flows,re_octets,re_pkts,re_flows,col_time) values(%d, %u, %u, %u, %u, '%s','%s',\
															%u,%u,%llu,%u,%u,%llu,%u,%u,'%s')\n",router_id, time_set.week,router_id,\
															app_temp->index, app_temp->port, app_temp->portid, app_temp->proto, sip, dip, \
															app_ip_detail_max.saddr[3], \
															app_ip_detail_max.daddr[3], \
															app_ip_detail_max.sd_octets, app_ip_detail_max.sd_pkts, app_ip_detail_max.sd_flows,\
															app_ip_detail_max.ds_octets, app_ip_detail_max.ds_pkts, app_ip_detail_max.ds_flows, time_set.time_str);
													}
												}
												else{
													if(VERSION_V6 == v_flag_s || VERSION_V6 == v_flag_d){
														fprintf(f2, "insert into r%d_app_detail_%d(r_id,if_index,port,port_flag,proto_id,src_ip,dst_ip,\
															src_ip_int0,src_ip_int1,src_ip_int2,src_ip_int,dst_ip_int0,dst_ip_int1,dst_ip_int2,dst_ip_int,\
															octets,pkts,flows,re_octets,re_pkts,re_flows,col_time) values(%d, %u, %u, %u, %u, '%s','%s',\
															%u,%u,%u,%u,%u,%u,%u,%u,%llu,%u,%u,%llu,%u,%u,'%s')\n",router_id, time_set.week,router_id,\
															app_temp->index, app_temp->port, app_temp->portid, app_temp->proto, dip, sip, \
															app_ip_detail_max.daddr[0],app_ip_detail_max.daddr[1],app_ip_detail_max.daddr[2],app_ip_detail_max.daddr[3],\
															app_ip_detail_max.saddr[0],app_ip_detail_max.saddr[1],app_ip_detail_max.saddr[2],app_ip_detail_max.saddr[3],\
															app_ip_detail_max.ds_octets, app_ip_detail_max.ds_pkts, app_ip_detail_max.ds_flows,\
															app_ip_detail_max.sd_octets, app_ip_detail_max.sd_pkts, app_ip_detail_max.sd_flows, time_set.time_str);
													}
													else if(VERSION_V4 == v_flag_s && VERSION_V4 == v_flag_d){
														fprintf(f2, "insert into r%d_app_detail_%d(r_id,if_index,port,port_flag,proto_id,src_ip,dst_ip,\
															src_ip_int,dst_ip_int,\
															octets,pkts,flows,re_octets,re_pkts,re_flows,col_time) values(%d, %u, %u, %u, %u, '%s','%s',\
															%u,%u,%llu,%u,%u,%llu,%u,%u,'%s')\n",router_id, time_set.week,router_id,\
															app_temp->index, app_temp->port, app_temp->portid, app_temp->proto, dip, sip, \
															app_ip_detail_max.daddr[3],\
															app_ip_detail_max.saddr[3],\
															app_ip_detail_max.ds_octets, app_ip_detail_max.ds_pkts, app_ip_detail_max.ds_flows,\
															app_ip_detail_max.sd_octets, app_ip_detail_max.sd_pkts, app_ip_detail_max.sd_flows, time_set.time_str);
													}
												}
												indel_flag = 1;
											}
											else if(1 == indel_flag){
												if(v_flag_s_old != v_flag_s || v_flag_d_old != v_flag_d){
													fprintf(f2, ";\n");
													indel_flag = 0;
													m--;
													continue;
												}
												if(-1 == ipv6_cmp(app_ip_detail_max.saddr, app_ip_detail_max.daddr)){
													if(VERSION_V6 == v_flag_s || VERSION_V6 == v_flag_d){
														fprintf(f2, ",(%d, %u, %u, %u, %u, '%s','%s',%u,%u,%u,%u,%u,%u,%u,%u,%llu,%u,%u,%llu,%u,%u,'%s'\n)",router_id,\
															app_temp->index, app_temp->port, app_temp->portid, app_temp->proto, sip, dip, \
															app_ip_detail_max.saddr[0],app_ip_detail_max.saddr[1],app_ip_detail_max.saddr[2],app_ip_detail_max.saddr[3], \
															app_ip_detail_max.daddr[0],app_ip_detail_max.daddr[1],app_ip_detail_max.daddr[2],app_ip_detail_max.daddr[3], \
															app_ip_detail_max.sd_octets, app_ip_detail_max.sd_pkts, app_ip_detail_max.sd_flows,\
															app_ip_detail_max.ds_octets, app_ip_detail_max.ds_pkts, app_ip_detail_max.ds_flows, time_set.time_str);
													}
													else if(VERSION_V4 == v_flag_s && VERSION_V4 == v_flag_d){
														fprintf(f2, ",(%d, %u, %u, %u, %u, '%s','%s',%u,%u,%llu,%u,%u,%llu,%u,%u,'%s'\n)",router_id,\
															app_temp->index, app_temp->port, app_temp->portid, app_temp->proto, sip, dip, \
															app_ip_detail_max.saddr[3], \
															app_ip_detail_max.daddr[3], \
															app_ip_detail_max.sd_octets, app_ip_detail_max.sd_pkts, app_ip_detail_max.sd_flows,\
															app_ip_detail_max.ds_octets, app_ip_detail_max.ds_pkts, app_ip_detail_max.ds_flows, time_set.time_str);
													}
												}
												else{
													if(VERSION_V6 == v_flag_s || VERSION_V6 == v_flag_d){
														fprintf(f2, ",(%d, %u, %u, %u, %u, '%s','%s',%u,%u,%u,%u,%u,%u,%u,%u,%llu,%u,%u,%llu,%u,%u,'%s'\n)",router_id,\
															app_temp->index, app_temp->port, app_temp->portid, app_temp->proto, dip, sip, \
															app_ip_detail_max.daddr[0],app_ip_detail_max.daddr[1],app_ip_detail_max.daddr[2],app_ip_detail_max.daddr[3],\
															app_ip_detail_max.saddr[0],app_ip_detail_max.saddr[1],app_ip_detail_max.saddr[2],app_ip_detail_max.saddr[3],\
															app_ip_detail_max.ds_octets, app_ip_detail_max.ds_pkts, app_ip_detail_max.ds_flows,\
															app_ip_detail_max.sd_octets, app_ip_detail_max.sd_pkts, app_ip_detail_max.sd_flows, time_set.time_str);
													}
													else if(VERSION_V4 == v_flag_s && VERSION_V4 == v_flag_d){
														fprintf(f2, ",(%d, %u, %u, %u, %u, '%s','%s',%u,%u,%llu,%u,%u,%llu,%u,%u,'%s'\n)",router_id,\
															app_temp->index, app_temp->port, app_temp->portid, app_temp->proto, dip, sip, \
															app_ip_detail_max.daddr[3],\
															app_ip_detail_max.saddr[3],\
															app_ip_detail_max.ds_octets, app_ip_detail_max.ds_pkts, app_ip_detail_max.ds_flows,\
															app_ip_detail_max.sd_octets, app_ip_detail_max.sd_pkts, app_ip_detail_max.sd_flows, time_set.time_str);
													}
												}
											}
											//change MAX into 0
											app_ip_detail_max.ip_next->sd_octets = 0;
											app_ip_detail_max.ip_next->ds_octets = 0;
										}
									}
									
									if (indel_flag == 1)
									fprintf(f2,";\n");
								}
							}
						}
					}
					break;
				}
			}
		}
	}
	
	fprintf(f2,"commit;\n");
	fclose(f2); 
 
	//data file is not empty
	if (file_stat > 0){
		sprintf(cmdline, "%s/SQLDETAIL_app_%s", SQL_COLLECT, time_set.tim);
		rename(filename2,cmdline);
	}
	else{	
		sprintf(cmdline, "rm -fr %s", filename2);
		system(cmdline);
	}
}

/********************************************************************
* Function: ft_app_clean   
* Description: clean struct array  
* Input: void
* Output: 
* Return: void
* Others: 
********************************************************************/
void ft_app_clean(){
	int i, j, k, m;
	struct app_t* app_temp, *pre_app;
	struct  app_ip_detail_t *app_ip_detail_temp,*pre_ip_detail;
	
	//free PORT link node and detail IP node
	for(i = 1; i <= max_rid_count; i++){
		for(j = 0; j < MAX_IF_COUNT; j++){
			if(NULL == app[i][j]){
				break;
			}
			for(k = 0; k < APP_COUNT; k++){
				app_temp = app[i][j]+k;
				
				while (app_temp->port_next != NULL){
					if(app_temp->ip_detail != NULL){
						//detail IP
						for (m = 0; m < APP_COUNT_DETAIL; m++){
							app_ip_detail_temp = app_temp->ip_detail + m;
							while (app_ip_detail_temp->ip_next != NULL){
								pre_ip_detail = app_ip_detail_temp;
								app_ip_detail_temp = app_ip_detail_temp->ip_next;
								
								if (pre_ip_detail != app_temp->ip_detail + m){ // is link node ,not array node
									free(pre_ip_detail);
									pre_ip_detail = NULL;
								}
							}
							if(NULL == app_ip_detail_temp->ip_next){
								if (app_ip_detail_temp != app_temp->ip_detail + m){ // is link node ,not array node
									free(app_ip_detail_temp);
									app_ip_detail_temp = NULL;
								}
							}
						}
						free(app_temp->ip_detail);
						app_temp->ip_detail = NULL;
					}
					//detail port
					pre_app = app_temp;
					app_temp = app_temp->port_next;
					if (pre_app != app[i][j]+k){ // is link node ,not array node
						free(pre_app);
						pre_app = NULL;
					}
				}
				if(NULL == app_temp->port_next){
					if (app_temp->ip_detail != NULL){
						//detail IP
						for (m = 0; m < APP_COUNT_DETAIL; m++){
							app_ip_detail_temp = app_temp->ip_detail + m;
							while (app_ip_detail_temp->ip_next != NULL){
								pre_ip_detail = app_ip_detail_temp;
								app_ip_detail_temp = app_ip_detail_temp->ip_next;
								
								if (pre_ip_detail != app_temp->ip_detail + m){ // is link node ,not array node
									free(pre_ip_detail);
									pre_ip_detail = NULL;
								}
							}
							if(NULL == app_ip_detail_temp->ip_next){
								if (app_ip_detail_temp != app_temp->ip_detail + m){ // is link node ,not array node
									free(app_ip_detail_temp);
									app_ip_detail_temp = NULL;
								}
							}
						}
						free(app_temp->ip_detail);
						app_temp->ip_detail = NULL;	
					}
					if (app_temp != app[i][j]+k){ // is link node ,not array node
						free(app_temp);
						app_temp = NULL;
					}
				}
			}
			
			free(app[i][j]);
			app[i][j] = NULL;
		}
	}
}

/********************************************************************
* Function: ft_ip_sort   
* Description: Analysis ip data£¬save data into the struct array
* Input: struct flow pdu
* Output: 
* Return: void
* Others: 
********************************************************************/
void ft_ip_sort(struct flow pdu){
	int i, j, k, srcpos, dstpos, de_srcpos, de_dstpos, pos;
	u_int16  count;
	struct ip_t *ipcur, *preip;
	struct ip_detail_t *right, *preright;
	count = pdu.rid;//id  of router  in  router's config file
	
	srcpos 		= pdu.saddr[3] % IP_NUM;
	dstpos 		= pdu.daddr[3] % IP_NUM;
	de_srcpos = pdu.saddr[3] % IP_DETAIL_NUM;
	de_dstpos = pdu.daddr[3] % IP_DETAIL_NUM;
	
	//inif: saddr
	for (i = 0; i < MAX_IF_COUNT; i++){
		if(NULL == ip_past[count][i]){
			ip_past[count][i] = (struct interface_ip *)malloc(sizeof(struct interface_ip));
			if (NULL == ip_past[count][i]){
				return;
			}
			memset(ip_past[count][i], 0, sizeof(struct interface_ip));
			ip_past[count][i]->ifindex = pdu.inif;
			
			if(1 == HAS_IP_DETAIL){
				//sign ip
				for (k = 0; k < MAX_IF_COUNT; k++){
					if(NULL == iptop[count][k]){
						break;
					}
					else if (iptop[count][k]->ifindex == pdu.inif){
						//innet ip copy
						for(j = 0; j < IP_TOPN; j++){
							pos = iptop[count][k]->addr_topin[j][3] % IP_NUM;
							
							if(0 == ip_past[count][i]->ip_in[pos].detail_flag){
			     			ip_past[count][i]->ip_in[pos].addr[0] 		= iptop[count][k]->addr_topin[j][0];
			     			ip_past[count][i]->ip_in[pos].addr[1] 		= iptop[count][k]->addr_topin[j][1];
			     			ip_past[count][i]->ip_in[pos].addr[2] 		= iptop[count][k]->addr_topin[j][2];
			     			ip_past[count][i]->ip_in[pos].addr[3] 		= iptop[count][k]->addr_topin[j][3];
			     			iptop[count][k]->topin_point[j] 					= &(ip_past[count][i]->ip_in[pos]);
								ip_past[count][i]->ip_in[pos].detail_flag = 1;
							}
							else{  //if (ip_past[k][n]->ip[pos].detail_flag == 1){
								ipcur = &(ip_past[count][i] ->ip_in[pos]);  
								while(ipcur->next != NULL){
									ipcur= ipcur->next;	
								}
								
			          if(NULL == ipcur->next){
									ipcur->next = (struct ip_t *)malloc(sizeof(struct ip_t));
									if(NULL == ipcur->next){
										return;
									}
									memset(ipcur->next, 0, sizeof(struct ip_t));
									ipcur->next->addr[0] 						= iptop[count][k]->addr_topin[j][0];
									ipcur->next->addr[1] 						= iptop[count][k]->addr_topin[j][1];
									ipcur->next->addr[2] 						= iptop[count][k]->addr_topin[j][2];
									ipcur->next->addr[3] 						= iptop[count][k]->addr_topin[j][3];
									iptop[count][k]->topin_point[j] = ipcur->next;
									ipcur->next->detail_flag 				= 1;
								}	
							}
						}
						
						//outnet IP copy
						for(j = 0; j<IP_TOPN; j++){
							pos = iptop[count][k]->addr_topout[j][3] % IP_NUM;
							
							if(0 == ip_past[count][i]->ip_out[pos].detail_flag) {
			     			ip_past[count][i]->ip_out[pos].addr[0] 			= iptop[count][k]->addr_topout[j][0];
			     			ip_past[count][i]->ip_out[pos].addr[1] 			= iptop[count][k]->addr_topout[j][1];
			     			ip_past[count][i]->ip_out[pos].addr[2] 			= iptop[count][k]->addr_topout[j][2];
			     			ip_past[count][i]->ip_out[pos].addr[3] 			= iptop[count][k]->addr_topout[j][3];
			     			iptop[count][k]->topout_point[j] 						= &(ip_past[count][i]->ip_out[pos]);
								ip_past[count][i]->ip_out[pos].detail_flag 	= 1;
							}
							else{ //if (ip_past[k][n]->ip[pos].detail_flag == 1)
								ipcur = &(ip_past[count][i] ->ip_out[pos]);  
								while(ipcur->next != NULL){
									ipcur= ipcur->next;	
								}
								
			          if(NULL == ipcur->next){
									ipcur->next = (struct ip_t *)malloc(sizeof(struct ip_t));
									if(NULL == ipcur->next){
										return;
									}
									memset(ipcur->next, 0, sizeof(struct ip_t));
									ipcur->next->addr[0] 							= iptop[count][k]->addr_topout[j][0];
									ipcur->next->addr[1] 							= iptop[count][k]->addr_topout[j][1];
									ipcur->next->addr[2] 							= iptop[count][k]->addr_topout[j][2];
									ipcur->next->addr[3] 							= iptop[count][k]->addr_topout[j][3];
									iptop[count][k]->topout_point[j] 	= ipcur->next;
									ipcur->next->detail_flag 					= 1;
								}		
							}
						}
						
						break;
					}
				}
			}
		}
		if(ip_past[count][i]->ifindex == pdu.inif){
			//inif: saddr
			if(0 == ip_past[count][i]->ip_in[srcpos].detail_flag){
				//nothing
			}
			else{
				ipcur = &(ip_past[count][i] ->ip_in[srcpos]);
				while(ipcur != NULL){
					if(0 != ipv6_cmp(ipcur->addr, pdu.saddr)){
						preip = ipcur;
						ipcur= ipcur->next;
						continue;
					}
					else{
						if(1 == ipcur->detail_flag){
							if(NULL == ipcur->ip_detail){
								ipcur->ip_detail = (struct ip_detail_t **)malloc(IP_DETAIL_NUM*sizeof(struct ip_detail_t *));	
								
								if(NULL == ipcur->ip_detail){
									return;
								}
								memset(ipcur->ip_detail, 0, IP_DETAIL_NUM*sizeof(struct ip_detail_t *));
							}
							if (NULL == *(ipcur->ip_detail+de_dstpos) && ipcur->ip_detail_total_count < IP_DETAIL_TOTAL_NUM){
								*(ipcur->ip_detail+de_dstpos) = (struct ip_detail_t *)malloc(sizeof(struct ip_detail_t));
								if (NULL == *(ipcur->ip_detail+de_dstpos)){
									return;
								}
								memset(*(ipcur->ip_detail+de_dstpos), 0, sizeof(struct ip_detail_t));	
								
								right = *(ipcur->ip_detail+de_dstpos);							
								right->addr[0] 		= pdu.daddr[0];
								right->addr[1] 		= pdu.daddr[1];
								right->addr[2]		= pdu.daddr[2];
								right->addr[3] 		= pdu.daddr[3];
								right->octets 		= pdu.octets;
								right->pkts 			= pdu.pkts;
								right->flows 			= 1;
								right->protol 		= pdu.proto;
								right->port 			= pdu.port;
								right->port_flag 	= pdu.portid;   
								right->detail_if 	= pdu.outif;
								
								ipcur->ip_detail_total_count++;
								break;
							}
							else{
								right = *(ipcur->ip_detail+de_dstpos);
								while (right != NULL){
									if(0 != ipv6_cmp(right->addr, pdu.daddr) || right->port != pdu.port || right->port_flag != pdu.portid || right->protol != pdu.proto){
										preright = right;
										right = right->next_detail;
										continue;
									} 
									else{
										right->octets += pdu.octets;
										right->pkts += pdu.pkts;
										right->flows++;
										break;
									}
								}
								
								if (NULL == right && ipcur->ip_detail_total_count < IP_DETAIL_TOTAL_NUM){
									right = (struct ip_detail_t *)malloc(sizeof(struct ip_detail_t));
									if(NULL == right){
										return;
									}
									memset(right, 0, sizeof(struct ip_detail_t));	
									right->addr[0] 					= pdu.daddr[0];
									right->addr[1] 					= pdu.daddr[1];
									right->addr[2] 					= pdu.daddr[2];
									right->addr[3] 					= pdu.daddr[3];
									right->octets 					= pdu.octets;
									right->pkts 						= pdu.pkts;
									right->flows 						= 1;
									right->protol 					= pdu.proto;
									right->port 						= pdu.port;
									right->port_flag 				= pdu.portid;  
									right->detail_if 				= pdu.outif;
									preright ->next_detail 	= right;
									
									ipcur->ip_detail_total_count++;
								}
								break;
							}
						}
						break;
					}
				}
			}
			
			//inif: daddr 
			if(0 == ip_past[count][i] ->ip_out[dstpos].detail_flag){
				//nothing
			}
			else{
				ipcur = &(ip_past[count][i]->ip_out[dstpos]);
				while (ipcur != NULL){
					if(0 != ipv6_cmp(ipcur->addr, pdu.daddr)){
						preip = ipcur;
						ipcur= ipcur->next;
						continue;
					}
					else{
						if(1 == ipcur->detail_flag){
							if(NULL == ipcur->ip_detail){
								ipcur->ip_detail = (struct ip_detail_t **)malloc(IP_DETAIL_NUM*sizeof(struct ip_detail_t *));	
								 if(NULL == ipcur->ip_detail){
									 return;
								 }
								 memset(ipcur->ip_detail, 0, IP_DETAIL_NUM * sizeof(struct ip_detail_t *));
							}
							if (NULL == *(ipcur->ip_detail+de_srcpos) && ipcur->ip_detail_total_count < IP_DETAIL_TOTAL_NUM){
								*(ipcur->ip_detail+de_srcpos) = (struct ip_detail_t *)malloc(sizeof(struct ip_detail_t));
								 if (NULL == *(ipcur->ip_detail+de_srcpos)){
									 return;
								 }
								 memset(*(ipcur->ip_detail+de_srcpos), 0, sizeof(struct ip_detail_t));	

								 right = *(ipcur->ip_detail+de_srcpos);
								 right->addr[0] 	= pdu.saddr[0];
								 right->addr[1] 	= pdu.saddr[1];
								 right->addr[2] 	= pdu.saddr[2];
								 right->addr[3] 	= pdu.saddr[3];
								 right->re_octets = pdu.octets;
								 right->re_pkts 	= pdu.pkts;
								 right->re_flows 	= 1;
								 right->protol 		= pdu.proto;
								 right->port 			= pdu.port;
								 right->port_flag = pdu.portid;  
								 ipcur->ip_detail_total_count++;
								 break;
							}
							else{
								right = *(ipcur->ip_detail+de_srcpos);
								while (right != NULL){
									if (0 != ipv6_cmp(right->addr, pdu.saddr) || right->port != pdu.port || right->port_flag != pdu.portid || right->protol != pdu.proto){
										preright = right;
										right = right->next_detail;
										continue;
									}
									else{
										right->re_octets += pdu.octets;
										right->re_pkts += pdu.pkts;
										right->re_flows++;
										break;
									}
								}
								
								if (right == NULL && ipcur->ip_detail_total_count<IP_DETAIL_TOTAL_NUM){
									right = (struct ip_detail_t *)malloc(sizeof(struct ip_detail_t));
									if(NULL == right){
										return;
									}
									memset(right, 0, sizeof(struct ip_detail_t));	
									right->addr[0] 					= pdu.saddr[0];
									right->addr[1] 					= pdu.saddr[1];
									right->addr[2] 					= pdu.saddr[2];
									right->addr[3] 					= pdu.saddr[3];
									right->re_octets 				= pdu.octets;
									right->re_pkts 					= pdu.pkts;
									right->re_flows 				= 1;
									right->protol 					= pdu.proto;
									right->port 						= pdu.port;
									right->port_flag 				= pdu.portid;  
									preright ->next_detail 	= right;
									
									ipcur->ip_detail_total_count++;
								}
								break;
							}
						}
						break;
					}
				}
				break;
			}
			break;
		}
	}
	
	//outif: daddr 
	for (i = 0; i < MAX_IF_COUNT; i++){
		if(NULL == ip_past[count][i]){
			ip_past[count][i] = (struct interface_ip *)malloc(sizeof(struct interface_ip));		
			if (NULL == ip_past[count][i]){
				return;
			}
			memset(ip_past[count][i], 0, sizeof(struct interface_ip));
			ip_past[count][i]->ifindex = pdu.outif;

			if(1 == HAS_IP_DETAIL){
				//sign ip
				for (k = 0; k < MAX_IF_COUNT; k++){
					if(NULL == iptop[count][k]){
						break;
					}
					else if (iptop[count][k]->ifindex == pdu.outif){
						//innet ip copy
						for(j = 0; j < IP_TOPN; j++){
							pos = iptop[count][k]->addr_topin[j][3] % IP_NUM;
							
							if(0 == ip_past[count][i]->ip_in[pos].detail_flag){
			     			ip_past[count][i]->ip_in[pos].addr[0] 		= iptop[count][k]->addr_topin[j][0];
			     			ip_past[count][i]->ip_in[pos].addr[1] 		= iptop[count][k]->addr_topin[j][1];
			     			ip_past[count][i]->ip_in[pos].addr[2] 		= iptop[count][k]->addr_topin[j][2];
			     			ip_past[count][i]->ip_in[pos].addr[3] 		= iptop[count][k]->addr_topin[j][3];
			     			iptop[count][k]->topin_point[j] 					= &(ip_past[count][i]->ip_in[pos]);
								ip_past[count][i]->ip_in[pos].detail_flag = 1;
							}		
							else{
								ipcur = &(ip_past[count][i] ->ip_in[pos]);  
								while (ipcur->next != NULL){
									ipcur= ipcur->next;	
								}
								
			          if (NULL == ipcur->next){
			          	ipcur->next = (struct ip_t *)malloc(sizeof(struct ip_t));
									if (NULL == ipcur->next){
										return;
									}
									memset(ipcur->next, 0, sizeof(struct ip_t));
									ipcur->next->addr[0] 						= iptop[count][k]->addr_topin[j][0];
									ipcur->next->addr[1] 						= iptop[count][k]->addr_topin[j][1];
									ipcur->next->addr[2] 						= iptop[count][k]->addr_topin[j][2];
									ipcur->next->addr[3] 						= iptop[count][k]->addr_topin[j][3];
									iptop[count][k]->topin_point[j] = ipcur->next;
									ipcur->next->detail_flag 				= 1;
			          }
							}
						}
						
						//outnet IP copy
						for(j = 0; j < IP_TOPN; j++){
							pos = iptop[count][k]->addr_topout[j][3] % IP_NUM;
							
							if(0 == ip_past[count][i]->ip_out[pos].detail_flag){
			     			ip_past[count][i]->ip_out[pos].addr[0] 			= iptop[count][k]->addr_topout[j][0];
			     			ip_past[count][i]->ip_out[pos].addr[1] 			= iptop[count][k]->addr_topout[j][1];
			     			ip_past[count][i]->ip_out[pos].addr[2] 			= iptop[count][k]->addr_topout[j][2];
			     			ip_past[count][i]->ip_out[pos].addr[3] 			= iptop[count][k]->addr_topout[j][3];
			     			iptop[count][k]->topout_point[j] 						= &(ip_past[count][i]->ip_out[pos]);
								ip_past[count][i]->ip_out[pos].detail_flag 	= 1;
							}
							
							else{
								ipcur = &(ip_past[count][i] ->ip_out[pos]);  
								while (ipcur->next != NULL){
									ipcur= ipcur->next;	
								}
								
			          if(NULL == ipcur->next){
									ipcur->next = (struct ip_t *)malloc(sizeof(struct ip_t));
									if(NULL == ipcur->next){
										return;
									}
									memset(ipcur->next, 0, sizeof(struct ip_t));
									ipcur->next->addr[0] = iptop[count][k]->addr_topout[j][0];
									ipcur->next->addr[1] = iptop[count][k]->addr_topout[j][1];
									ipcur->next->addr[2] = iptop[count][k]->addr_topout[j][2];
									ipcur->next->addr[3] = iptop[count][k]->addr_topout[j][3];
									iptop[count][k]->topout_point[j] = ipcur->next;
									ipcur->next->detail_flag = 1;
								}
							}
						}
						
						break;
					}
				}
			}
		}
		
		if(ip_past[count][i]->ifindex == pdu.outif){
			//outif: daddr
			if(0 == ip_past[count][i]->ip_in[dstpos].detail_flag){
				//nothing
			}
			else{
				ipcur = &(ip_past[count][i]->ip_in[dstpos]);
				while (ipcur != NULL){
					if(0 != ipv6_cmp(ipcur->addr, pdu.daddr)){
						preip = ipcur;
						ipcur= ipcur->next;
						continue;
					}
					else{
						if(1 == ipcur->detail_flag){
							if(NULL == ipcur->ip_detail){
								ipcur->ip_detail = (struct ip_detail_t **)malloc(IP_DETAIL_NUM*sizeof(struct ip_detail_t *));
								if(NULL == ipcur->ip_detail){
									return;
								}
								memset(ipcur->ip_detail, 0, IP_DETAIL_NUM*sizeof(struct ip_detail_t *));
							}
							if (NULL == *(ipcur->ip_detail+de_srcpos) && ipcur->ip_detail_total_count < IP_DETAIL_TOTAL_NUM){
								*(ipcur->ip_detail+de_srcpos) = (struct ip_detail_t *)malloc(sizeof(struct ip_detail_t));
								if (NULL == *(ipcur->ip_detail+de_srcpos)){
									return;
								}
								memset(*(ipcur->ip_detail+de_srcpos), 0, sizeof(struct ip_detail_t));	

								right = *(ipcur->ip_detail+de_srcpos);
								right->addr[0] 		= pdu.saddr[0];
								right->addr[1] 		= pdu.saddr[1];
								right->addr[2] 		= pdu.saddr[2];
								right->addr[3] 		= pdu.saddr[3];
								right->re_octets 	= pdu.octets;
								right->re_pkts 		= pdu.pkts;
								right->re_flows 	= 1;
								right->protol 		= pdu.proto;
								right->port 			= pdu.port;
								right->port_flag 	= pdu.portid;
								right->detail_if 	= pdu.inif;
								ipcur->ip_detail_total_count++;
								break;
							}
							else{
								right = *(ipcur->ip_detail+de_srcpos);
								while (right != NULL){
									if(0 != ipv6_cmp(right->addr, pdu.saddr) || right->port != pdu.port || right->port_flag != pdu.portid || right->protol != pdu.proto){
										preright = right;
										right = right->next_detail;
										continue;
									}
									else{
										right->re_octets += pdu.octets;
										right->re_pkts += pdu.pkts;
										right->re_flows++;
										break;
									}
								}
								
								if(NULL == right && ipcur->ip_detail_total_count < IP_DETAIL_TOTAL_NUM){
									right = (struct ip_detail_t *)malloc(sizeof(struct ip_detail_t));
									if(NULL == right){
										return;
									}
									memset(right, 0, sizeof(struct ip_detail_t));	
									right->addr[0] 					= pdu.saddr[0];
									right->addr[1] 					= pdu.saddr[1];
									right->addr[2] 					= pdu.saddr[2];
									right->addr[3] 					= pdu.saddr[3];
									right->re_octets 				= pdu.octets;
									right->re_pkts 					= pdu.pkts;
									right->re_flows 				= 1;
									right->protol 					= pdu.proto;
									right->port 						= pdu.port;
									right->port_flag 				= pdu.portid; 
									right->detail_if 				= pdu.inif;
									preright ->next_detail 	= right;								
									ipcur->ip_detail_total_count++;
								}
								break;
							}
						}
						break;
					}
				}
			}
			
			//outif: saddr 
			if(0 == ip_past[count][i]->ip_out[srcpos].detail_flag){
				//nothing
			}
			else{
				ipcur = &(ip_past[count][i] ->ip_out[srcpos]);
				while (ipcur != NULL){
					if(0 != ipv6_cmp(ipcur->addr, pdu.saddr)){
						preip = ipcur;
						ipcur= ipcur->next;
						continue;
					}
					else{
						if(1 == ipcur->detail_flag){
							if(NULL == ipcur->ip_detail){
								ipcur->ip_detail = (struct ip_detail_t **)malloc(IP_DETAIL_NUM*sizeof(struct ip_detail_t *));	
								if(NULL == ipcur->ip_detail){
									return;
								}
								memset(ipcur->ip_detail, 0, IP_DETAIL_NUM*sizeof(struct ip_detail_t *));
							}
							if(NULL == *(ipcur->ip_detail+de_dstpos) && ipcur->ip_detail_total_count < IP_DETAIL_TOTAL_NUM){
								*(ipcur->ip_detail+de_dstpos) = (struct ip_detail_t *)malloc(sizeof(struct ip_detail_t));
								if (NULL == *(ipcur->ip_detail+de_dstpos)){
									return;
								}
								memset(*(ipcur->ip_detail+de_dstpos), 0, sizeof(struct ip_detail_t));	

								right = *(ipcur->ip_detail+de_dstpos);
								right->addr[0] 		= pdu.daddr[0];
								right->addr[1] 		= pdu.daddr[1];
								right->addr[2] 		= pdu.daddr[2];
								right->addr[3] 		= pdu.daddr[3];
								right->octets 		= pdu.octets;
								right->pkts 			= pdu.pkts;
								right->flows 			= 1;
								right->protol 		= pdu.proto;
								right->port 			= pdu.port;
								right->port_flag 	= pdu.portid;  
								ipcur->ip_detail_total_count++;
								break;
							}
							else{
								right = *(ipcur->ip_detail+de_dstpos);
								while(right != NULL){
									if(0 != ipv6_cmp(right->addr, pdu.daddr) || right->port != pdu.port || right->port_flag != pdu.portid || right->protol != pdu.proto){
										preright = right;
										right = right->next_detail;
										continue;
									} 
									else{
										right->octets += pdu.octets;
										right->pkts += pdu.pkts;
										right->flows++;
										break;
									}
								}
								if (NULL == right && ipcur->ip_detail_total_count < IP_DETAIL_TOTAL_NUM){
									right = (struct ip_detail_t *)malloc(sizeof(struct ip_detail_t));
									if(NULL == right){
										return;
									}
									memset(right, 0, sizeof(struct ip_detail_t));	
									right->addr[0] 					= pdu.daddr[0];
									right->addr[1] 					= pdu.daddr[1];
									right->addr[2] 					= pdu.daddr[2];
									right->addr[3] 					= pdu.daddr[3];
									right->octets 					= pdu.octets;
									right->pkts 						= pdu.pkts;
									right->flows 						= 1;
									right->protol 					= pdu.proto;
									right->port 						= pdu.port;
									right->port_flag 				= pdu.portid;
									preright ->next_detail 	= right; 
									ipcur->ip_detail_total_count++;
								}
								break;
							}
						}
						break;
					}
				}
				break;
			}
			break;
		}
	}
}

/********************************************************************
* Function: ft_ip_detail_topn   
* Description: Order by octets of ip detail analysis data
* Input: struct ip_t *
* Output: 
* Return: void
* Others: 
********************************************************************/
void ft_ip_detail_topn(struct ip_t *ipcur){
	int m, link_count;
	struct ip_detail_t *head, *prehead,*headcur,*tmphead,*predetail,*detailcur;
	head = NULL;
	link_count = 0;
	for (m = 0; m < IP_DETAIL_NUM; m++){
		if(NULL == ipcur->ip_detail){
			return;
		}
		if(NULL == *(ipcur->ip_detail+m)){
			continue;
		} 	
		detailcur = *(ipcur->ip_detail+m);
		
		while (detailcur != NULL){
			predetail = detailcur;
			detailcur = detailcur->next_detail;
			headcur = head;
			
			while((headcur != NULL)&&(headcur->octets+headcur->re_octets < predetail->octets + predetail->re_octets)){
				prehead = headcur;
				headcur = headcur->next_detail;
			}
			
			if(headcur == head){
				if(link_count < DETAIL_TOPN){
					predetail->next_detail = head;
					head = predetail;
					link_count++;
				}
				else{
					free(predetail);
				}
			}
			else{
				predetail->next_detail = headcur;
				prehead->next_detail = predetail;
				if(link_count < DETAIL_TOPN){
					link_count++;
				}
				else{
					tmphead = head;
					head = head->next_detail;
					free(tmphead);
				}
			}
		}
	}
	*(ipcur->ip_detail) = head;
}

/********************************************************************
* Function: ft_ip_detail_topn   
* Description: Order by octets of ip analysis data
* Input: struct ip_t *
* Output: 
* Return: void
* Others: 
********************************************************************/
void ft_ip_topn(void){
	int i, j, k;

	//ip topn
	for(k = 1; k <= max_rid_count; k++){
		for (i = 0; i < MAX_IF_COUNT; i++){
			if(NULL == ip_past[k][i] || NULL == iptop[k][i]){
				break;
			}
			//innet detail TOPN    
			for(j = 0; j < TOTAL_TOPN; j++){
			  if(NULL == iptop[k][i]->topin_point[j]){
				  break; 
			  }   
			  iptop[k][i]->addr_topin[j][0] =  iptop[k][i]->topin_point[j]->addr[0];
			  iptop[k][i]->addr_topin[j][1] =  iptop[k][i]->topin_point[j]->addr[1];
			  iptop[k][i]->addr_topin[j][2] =  iptop[k][i]->topin_point[j]->addr[2];
			  iptop[k][i]->addr_topin[j][3] =  iptop[k][i]->topin_point[j]->addr[3];
			  
			  if(1 == iptop[k][i]->topin_point[j]->detail_flag){
				  ft_ip_detail_topn(iptop[k][i]->topin_point[j]);
			  }	
			}
			//outnet detail TOPN	  
			for(j = 0; j < TOTAL_TOPN; j++){
			  if(NULL == iptop[k][i]->topout_point[j]){
				  break; 
			  }   
			  iptop[k][i]->addr_topout[j][0] =  iptop[k][i]->topout_point[j]->addr[0];
			  iptop[k][i]->addr_topout[j][1] =  iptop[k][i]->topout_point[j]->addr[1];
			  iptop[k][i]->addr_topout[j][2] =  iptop[k][i]->topout_point[j]->addr[2];
			  iptop[k][i]->addr_topout[j][3] =  iptop[k][i]->topout_point[j]->addr[3];
			  
			  if(1 == iptop[k][i]->topout_point[j]->detail_flag){
				  ft_ip_detail_topn(iptop[k][i]->topout_point[j]);
			  }	
			}
    }
  }
}

/********************************************************************
* Function: ft_ip_indb   
* Description: Write application  analysis data of the struct array into file 
* Input: void
* Output: 
* Return: int
* Others: 
********************************************************************/
int ft_ip_indb(void){
	FILE *f2;
	int  k, i, j,  insert_flag2, detail_file=0;
	char tmp_str1[128], tmp_str2[128], filename2[128],cmdline[256];

	struct ip_detail_t *detailcur, *predetail;
	int v_flag_s, v_flag_s_old;
	int v_flag_d, v_flag_d_old;
	
	
	sprintf(filename2, "%s/ip_detail_tmp.%s", TMPDATAPATH, time_set.tim);
	f2 = fopen(filename2, "w");
  if (NULL == f2){
  	write_log("ft_ip_indb() open ip_detail_tmp failed");
	  return -1;
  }
  fprintf(f2, "use forceviewfa;\nset autocommit=0;\n");
  
  for(k = 1; k <= max_rid_count; k++){
  	for (i = 0; i < MAX_IF_COUNT; i++){
  		if(NULL == iptop[k][i])			{
			  break;
			}
			
			//innet 
			insert_flag2 = 0;
			for(j = 0; j < IP_TOPN; j++){
				if(NULL == iptop[k][i]->topin_point[j]){
					break;
				}
				
				v_flag_s = ipv6_ntoa(iptop[k][i]->topin_point[j]->addr, tmp_str1, 128);
				//innet detail
				if(1 == iptop[k][i]->topin_point[j]->detail_flag){
					detail_file = 1;
					
					if(NULL == iptop[k][i]->topin_point[j]->ip_detail){
						continue;
					}
					detailcur = *(iptop[k][i]->topin_point[j]->ip_detail);
					while(detailcur != NULL){
						v_flag_d = ipv6_ntoa(detailcur->addr, tmp_str2, 128);
						
						if(0 == insert_flag2){
							v_flag_s_old = v_flag_s;
							v_flag_d_old = v_flag_d;
							if(VERSION_V6 == v_flag_s || VERSION_V6 == v_flag_d){
								fprintf(f2, "insert into r%d_ip_detail_innet_%d(r_id,if_index,src_ip,src_ip_int0,src_ip_int1,src_ip_int2,src_ip_int, \
									dst_ip,dst_ip_int0,dst_ip_int1,dst_ip_int2,dst_ip_int, port, port_flag, proto_id, octets, re_octets,\
									pkts,re_pkts,flows, re_flows,col_time) values(%d, %u, '%s', %u, %u, %u, %u, '%s', %u, %u, %u, %u, %u,%u, %u,%llu,%llu,%u,%u,%u,%u, '%s')\n", k, \
									time_set.week, k, iptop[k][i]->ifindex,\
									tmp_str1, iptop[k][i]->topin_point[j]->addr[0],iptop[k][i]->topin_point[j]->addr[1],iptop[k][i]->topin_point[j]->addr[2],iptop[k][i]->topin_point[j]->addr[3], \
									tmp_str2, detailcur->addr[0],detailcur->addr[1],detailcur->addr[2],detailcur->addr[3], \
									detailcur->port, detailcur->port_flag, \
									detailcur->protol, detailcur->octets, detailcur->re_octets, detailcur->pkts, \
									detailcur->re_pkts, detailcur->flows, detailcur->re_flows, time_set.time_str);	
							}
							else if(VERSION_V4 == v_flag_s && VERSION_V4 == v_flag_d){
								fprintf(f2, "insert into r%d_ip_detail_innet_%d(r_id,if_index,src_ip,src_ip_int, \
									dst_ip,dst_ip_int, port, port_flag, proto_id, octets, re_octets,\
									pkts,re_pkts,flows, re_flows,col_time) values(%d, %u, '%s', %u, '%s', %u, %u,%u, %u,%llu,%llu,%u,%u,%u,%u, '%s')\n", k, \
									time_set.week, k, iptop[k][i]->ifindex,\
									tmp_str1, iptop[k][i]->topin_point[j]->addr[3], \
									tmp_str2, detailcur->addr[3], \
									detailcur->port, detailcur->port_flag, \
									detailcur->protol, detailcur->octets, detailcur->re_octets, detailcur->pkts, \
									detailcur->re_pkts, detailcur->flows, detailcur->re_flows, time_set.time_str);
							}
							
							insert_flag2 = 1;
						}
						else{
							if(v_flag_s_old != v_flag_s || v_flag_d_old != v_flag_d){
								fprintf(f2, ";\n");
								insert_flag2 = 0;
								continue;
							}
							if(VERSION_V6 == v_flag_s || VERSION_V6 == v_flag_d){
								fprintf(f2, ",(%d, %u, '%s', %u, %u, %u, %u, '%s', %u, %u, %u, %u, %u,%u, %u,%llu,%llu,%u,%u,%u,%u, '%s')\n", k, iptop[k][i]->ifindex, \
									tmp_str1, iptop[k][i]->topin_point[j]->addr[0],iptop[k][i]->topin_point[j]->addr[1],iptop[k][i]->topin_point[j]->addr[2],iptop[k][i]->topin_point[j]->addr[3], \
									tmp_str2, detailcur->addr[0],detailcur->addr[1],detailcur->addr[2],detailcur->addr[3], \
									detailcur->port, detailcur->port_flag, \
									detailcur->protol, detailcur->octets, detailcur->re_octets, detailcur->pkts, \
									detailcur->re_pkts, detailcur->flows, detailcur->re_flows, time_set.time_str);
							}
							else if(VERSION_V4 == v_flag_s && VERSION_V4 == v_flag_d){
								fprintf(f2, ",(%d, %u, '%s', %u, '%s', %u, %u,%u, %u,%llu,%llu,%u,%u,%u,%u, '%s')\n", k, iptop[k][i]->ifindex, \
									tmp_str1, iptop[k][i]->topin_point[j]->addr[3], \
									tmp_str2, detailcur->addr[3], \
									detailcur->port, detailcur->port_flag, \
									detailcur->protol, detailcur->octets, detailcur->re_octets, detailcur->pkts, \
									detailcur->re_pkts, detailcur->flows, detailcur->re_flows, time_set.time_str);
							}
						}
						predetail = detailcur;
						detailcur = detailcur->next_detail;
						free(predetail);
					}
					free(iptop[k][i]->topin_point[j]->ip_detail);
					iptop[k][i]->topin_point[j]->ip_detail = NULL;
				}
			}
			if(1 == insert_flag2){
				fprintf(f2,";\n");
			}
			
			//outnet 
			insert_flag2 = 0;
			for(j = 0; j < IP_TOPN; j++){
				if(NULL == iptop[k][i]->topout_point[j]){
					break;
				}
				v_flag_s = ipv6_ntoa(iptop[k][i]->topout_point[j]->addr, tmp_str1, 128);
				//outnet detail
				if(1 == iptop[k][i]->topout_point[j]->detail_flag){
					detail_file = 1;
					
					if(NULL == iptop[k][i]->topout_point[j]->ip_detail){
						continue;
					}
					detailcur = *(iptop[k][i]->topout_point[j]->ip_detail);
					while (detailcur != NULL){
						v_flag_d = ipv6_ntoa(detailcur ->addr, tmp_str2, 128);
						
						if(0 == insert_flag2){
							v_flag_s_old = v_flag_s;
							v_flag_d_old = v_flag_d;
							if(VERSION_V6 == v_flag_s || VERSION_V6 == v_flag_d){
								fprintf(f2, "insert into r%d_ip_detail_outnet_%d(r_id,if_index,src_ip,src_ip_int0,src_ip_int1,src_ip_int2,src_ip_int, \
									dst_ip,dst_ip_int0,dst_ip_int1,dst_ip_int2,dst_ip_int, port, port_flag, proto_id, octets, re_octets,\
									pkts,re_pkts,flows, re_flows,col_time) values(%d, %u, '%s', %u, %u, %u, %u, '%s', %u, %u, %u, %u, %u,%u, %u,%llu,%llu,%u,%u,%u,%u, '%s')\n", k, \
									time_set.week, k, iptop[k][i]->ifindex, \
									tmp_str1, iptop[k][i]->topout_point[j]->addr[0],iptop[k][i]->topout_point[j]->addr[1],iptop[k][i]->topout_point[j]->addr[2],iptop[k][i]->topout_point[j]->addr[3], \
									tmp_str2, detailcur ->addr[0],detailcur ->addr[1],detailcur ->addr[2],detailcur ->addr[3], \
									detailcur->port, detailcur->port_flag, \
									detailcur->protol, detailcur->octets, detailcur->re_octets, detailcur->pkts, \
									detailcur->re_pkts, detailcur->flows, detailcur->re_flows, time_set.time_str);	
							}
							else if(VERSION_V4 == v_flag_s && VERSION_V4 == v_flag_d){
								fprintf(f2, "insert into r%d_ip_detail_outnet_%d(r_id,if_index,src_ip,src_ip_int, \
									dst_ip,dst_ip_int, port, port_flag, proto_id, octets, re_octets,\
									pkts,re_pkts,flows, re_flows,col_time) values(%d, %u, '%s', %u, '%s', %u, %u,%u, %u,%llu,%llu,%u,%u,%u,%u, '%s')\n", k, \
									time_set.week, k, iptop[k][i]->ifindex, \
									tmp_str1, iptop[k][i]->topout_point[j]->addr[3], \
									tmp_str2, detailcur ->addr[3], \
									detailcur->port, detailcur->port_flag, \
									detailcur->protol, detailcur->octets, detailcur->re_octets, detailcur->pkts, \
									detailcur->re_pkts, detailcur->flows, detailcur->re_flows, time_set.time_str);	
							}
							insert_flag2 = 1;
						}
						else{
							if(v_flag_s_old != v_flag_s || v_flag_d_old != v_flag_d){
								fprintf(f2, ";\n");
								insert_flag2 = 0;
								continue;
							}
							if(VERSION_V6 == v_flag_s || VERSION_V6 == v_flag_d){
								fprintf(f2, ",(%d, %u, '%s', %u, %u, %u, %u, '%s', %u, %u, %u, %u, %u,%u, %u,%llu,%llu,%u,%u,%u,%u, '%s')\n", k, iptop[k][i]->ifindex,\
									tmp_str1, iptop[k][i]->topout_point[j]->addr[0],iptop[k][i]->topout_point[j]->addr[1],iptop[k][i]->topout_point[j]->addr[2],iptop[k][i]->topout_point[j]->addr[3], \
									tmp_str2, detailcur ->addr[0],detailcur ->addr[1],detailcur ->addr[2],detailcur ->addr[3], \
									detailcur->port, detailcur->port_flag, \
									detailcur->protol, detailcur->octets, detailcur->re_octets, detailcur->pkts, \
									detailcur->re_pkts, detailcur->flows, detailcur->re_flows, time_set.time_str);	
							}
							else if(VERSION_V4 == v_flag_s && VERSION_V4 == v_flag_d){
								fprintf(f2, ",(%d, %u, '%s', %u, '%s', %u, %u,%u, %u,%llu,%llu,%u,%u,%u,%u, '%s')\n", k, iptop[k][i]->ifindex,\
									tmp_str1, iptop[k][i]->topout_point[j]->addr[3], \
									tmp_str2, detailcur ->addr[3], \
									detailcur->port, detailcur->port_flag, \
									detailcur->protol, detailcur->octets, detailcur->re_octets, detailcur->pkts, \
									detailcur->re_pkts, detailcur->flows, detailcur->re_flows, time_set.time_str);	
							}
						}
						predetail = detailcur;
						detailcur = detailcur->next_detail;
						free(predetail);
					}
					free(iptop[k][i]->topout_point[j]->ip_detail);
					iptop[k][i]->topout_point[j]->ip_detail = NULL;
				}
			}
			if(1 == insert_flag2){
				fprintf(f2,";\n");
			}
  	}
  }
  
  fprintf(f2,"commit;\n");
	fclose(f2);
	if (detail_file > 0){
		sprintf(cmdline, "%s/SQLDETAIL_ip_%s", SQL_COLLECT, time_set.tim);
		rename(filename2,cmdline);
	}
	else{
		sprintf(cmdline, "rm -fr %s", filename2);
		system(cmdline);
	}
	return 0;
}

/********************************************************************
* Function: ft_ip_clean   
* Description: clean struct array  
* Input: void
* Output: 
* Return: void
* Others: 
********************************************************************/
void ft_ip_clean(void){
	int i, j, k,m;
	struct ip_t *ipcur, *preip;
	struct ip_detail_t *detailcur, *predetail;
	for (k = 1; k <= max_rid_count; k++){
		for (i= 0; i < MAX_IF_COUNT; i++){
			if(NULL == ip_past[k][i]){
				break;
			}
			//innet
			for(j = 0; j< IP_NUM; j++){
				if(IPV6_ISZERO(ip_past[k][i]->ip_in[j].addr) && j > 0){
					continue;
				}
				ipcur = &(ip_past[k][i]->ip_in[j]);
				while (ipcur != NULL){
					if(ipcur->ip_detail != NULL){
						for (m = 0; m < IP_DETAIL_NUM; m++){
							if (NULL == *(ipcur->ip_detail + m)){
								continue;
							}
							detailcur = *(ipcur->ip_detail + m);
							while(detailcur != NULL){
								predetail = detailcur;
								detailcur = detailcur->next_detail;
								free(predetail);
							}
						}
						free(ipcur->ip_detail);
					}
					if (ipcur == &(ip_past[k][i]->ip_in[j])){
						ipcur = ipcur->next;
					}
					else{
						preip = ipcur;
						ipcur = ipcur->next;
						free(preip);
					}
				}
			}
			//outnet
			for(j = 0; j< IP_NUM; j++){
				if(IPV6_ISZERO(ip_past[k][i]->ip_out[j].addr) && j > 0){
					continue;	
				}
				ipcur = &(ip_past[k][i]->ip_out[j]);
				while (ipcur != NULL){
					if(ipcur->ip_detail != NULL){
						for (m = 0; m < IP_DETAIL_NUM; m++){
							if(NULL == *(ipcur->ip_detail + m)){
								continue;
							}
							detailcur = *(ipcur->ip_detail + m);
							while(detailcur != NULL){
								predetail = detailcur;
								detailcur = detailcur->next_detail;
								free(predetail);
							}
						}
						free(ipcur->ip_detail);
					}
					
					if (ipcur == &(ip_past[k][i]->ip_out[j])){
						ipcur = ipcur->next;
					}
					else{
						preip = ipcur;
						ipcur = ipcur->next;
						free(preip);
					}
				}
			}
			free(ip_past[k][i]);
		}
	}
	memset(ip_past, 0, MAX_IF_COUNT * MAX_ROUTER_COUNT * sizeof(struct interface_ip *));
}

int write_log(char *log_msg){
	time_t time1;
	struct tm m_tm;
	char time_str[128],filename[128];
	FILE *f1;
	
	time(&time1);
	localtime_r(&time1, &m_tm);
	sprintf(time_str, "%04d-%02d-%02d %02d:%02d:00", m_tm.tm_year+1900, m_tm.tm_mon+1, m_tm.tm_mday, m_tm.tm_hour, m_tm.tm_min);
	
	sprintf(filename, "/data/log/r_detail.log");
	f1 = fopen(filename, "a+");
	if (NULL == f1){
		return -1;
	}
	fprintf(f1, "%s   %s\n", time_str, log_msg);
	
	fclose(f1);
	return 0;
}

int version_info(void){
	FILE *fp;
	char content[1024];
	struct stat sbuf;
	
	memset(&sbuf,0,sizeof(struct stat));
	if (stat(VERSION_PATH, &sbuf) < 0){
		write_log("stat version.xml error");
		return -1;
	}

	if(version_file_time != sbuf.st_mtime){
		fp = fopen(VERSION_PATH, "r");
		if (NULL == fp){
			write_log("open version.xml failed");
			return -1;
		}
		
		//get time_interval
		while(fgets(content,256,fp) != NULL){
			if (strstr(content,"TimeInterval_m_router") != NULL){
				sscanf(content, "%*[^>]>%d", &TIME_INTERVAL);  
			}
			else if (strstr(content,"topN_total_router") != NULL){
				sscanf(content, "%*[^>]>%d", &TOTAL_TOPN);  
			}
			else if (strstr(content,"topN_detail_router") != NULL){
				sscanf(content, "%*[^>]>%d", &DETAIL_TOPN);  
			}
			else if (strstr(content,"hasdrill_ip_router") != NULL){
				sscanf(content, "%*[^>]>%d", &HAS_IP_DETAIL);  
			}
			else if (strstr(content,"hasdrill_app_router") != NULL){
				sscanf(content, "%*[^>]>%d", &HAS_APP_DETAIL);  
			}
			else if(strstr(content,"ip_all>") != NULL){
				sscanf(content,"%*[^>]>%d", &IP_ALL_FLAG);
			}
		}
		
		version_file_time = sbuf.st_mtime;
		fclose(fp);
		if(50 == TOTAL_TOPN && 50 == DETAIL_TOPN){
			topn_change_flag = 0;
		}
		else{
			topn_change_flag = 1;
		}		
	}
	return 0;
}

/********************************************************************
* Function: ipv6_ntoa   
* Description: ipv6 int translate into string 
* Input: const unsigned int,  char *, int
* Output: char *
* Return: int
* Others: 
********************************************************************/
static int ipv6_ntoa(const unsigned int *ipint, char* ipstr, int len){
	memset(ipstr, 0, len);
	if(0 == ipint[0] && 0 == ipint[1] && 0 == ipint[2]){
		sprintf(ipstr, "%d.%d.%d.%d", ipint[3]>>24, (ipint[3]<<8)>>24, (ipint[3]<<16)>>24,(ipint[3]<<24)>>24);
		return VERSION_V4;
	}
	else{
		sprintf(ipstr,"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",ipint[0]>>16,(ipint[0]<<16)>>16,ipint[1]>>16,(ipint[1]<<16)>>16,ipint[2]>>16,(ipint[2]<<16)>>16,ipint[3]>>16,(ipint[3]<<16)>>16);
		return VERSION_V6;
	}
	return 0;
}

/********************************************************************
* Function: ipv6_cmp   
* Description: ipv6 compare
* Input: const unsigned int,  const unsigned int,
* Output: 
* Return: int  1 ip1>ip2 | 0 ip1=ip2 | -1 ip1<ip2
* Others: 
********************************************************************/
static int ipv6_cmp(const unsigned int *ip1,const unsigned int *ip2){
	if(ip1[0] > ip2[0]){
		return 1;	
	}
	else if(ip1[0] < ip2[0]){
		return -1;
	}
	if(ip1[1] > ip2[1]){
		return 1;	
	}
	else if(ip1[1] < ip2[1]){
		return -1;
	}
	if(ip1[2] > ip2[2]){
		return 1;	
	}
	else if(ip1[2] < ip2[2]){
		return -1;
	}
	if(ip1[3] > ip2[3]){
		return 1;	
	}
	else if(ip1[3] < ip2[3]){
		return -1;
	}
	return 0;
}
