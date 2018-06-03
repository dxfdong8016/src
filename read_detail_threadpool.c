
#include "read_detail_threadpool.h"

//各个match函数
//设备分析-->接口

/*
void interface_match(void *myparam)
{
	int i=*(int*)myparam;
	char file_read[160]={0};
	char file_write[160]={0};
	char command[128]={0};
	sprintf(command,"/bin/zcat %s/%s >> %s/z%s",file_names[i].absolutepath,file_names[i].detailfile,file_names[i].tempfilepath,file_names[i].tempfile);
	printf("%s\n",command);
	system(command);
	sprintf(file_read,"%s/z%s",file_names[i].tempfilepath,file_names[i].tempfile);
	sprintf(file_write,"%s/%s",file_names[i].tempfilepath,file_names[i].tempfile);
	FILE * file_read_p=fopen(file_read,"r");
	FILE * file_write_p=fopen(file_write,"w+");
	if(!file_read_p||!file_write_p)
		return ;
	char linebuff[240]={0};
	struct flow pdu;
	printf("file_names[i].mymatch.myif=%d,file_names[i].mymatch.myif=%d\n",file_names[i].mymatch.myif,file_names[i].mymatch.myif);
	while(fgets(linebuff,240,file_read_p))
	{
		memset(&pdu,0,sizeof(pdu));
		sscanf(linebuff,"%d%*[|]%d%*[|]%d%*[|]%d%*[|]%d%*[|]%hu%*[|]%hu%*[|]%hu%*[|]%hu%*[|]%u%*[|]%u%*[|]%u%*[|]%u%*[|]%u%*[|]%u%*[|]%u%*[|]%hu%*[|]%hu%*[|]%hu%*[|]%c%*[|]%c%*[|]%c%*[|]%c%*[|]%c%*[|]%c%*[|]%c%*[|]%c", 
		                &pdu.rid, &pdu.mid, &pdu.hits, &pdu.saddr, &pdu.daddr, &pdu.srcas, &pdu.dstas, &pdu.sasgid, &pdu.dasgid, &pdu.nexthop, &pdu.inif, &pdu.outif, &pdu.first, &pdu.last,&pdu.pkts, &pdu.octets, &pdu.sport,
		                &pdu.dport, &pdu.port, &pdu.tcpflags, &pdu.proto, &pdu.tos, &pdu.srcmask, &pdu.dstmask, &pdu.pktid, &pdu.portid, &pdu.direct);
		if(pdu.inif ==file_names[i].mymatch.myif || pdu.outif == file_names[i].mymatch.myif)
		{
			fprintf(file_write_p,"%hu|%c|%c|%u|%u|%u|%u|%c|%c|%hu|%u|%u|%hu|%hu|%hu|%hu|%u|%u|%u|%hu|%hu|%c|%c|%c|%c|%c|%c\n",
			                      pdu.port, pdu.portid, pdu.proto, pdu.saddr, pdu.daddr, pdu.octets, pdu.pkts, pdu.rid, pdu.mid, pdu.hits, pdu.srcas, 
			                      pdu.dstas, pdu.sasgid, pdu.dasgid, pdu.nexthop, pdu.inif, pdu.outif, pdu.first, pdu.last,  pdu.sport, 
			                      pdu.dport, pdu.tcpflags, pdu.tos, pdu.srcmask, pdu.dstmask, pdu.pktid,  pdu.direct);
			printf("%hu|%c|%c|%u|%u|%u|%u|%c|%c|%hu|%u|%u|%hu|%hu|%hu|%hu|%u|%u|%u|%hu|%hu|%c|%c|%c|%c|%c|%c\n",
			                      pdu.port, pdu.portid, pdu.proto, pdu.saddr, pdu.daddr, pdu.octets, pdu.pkts, pdu.rid, pdu.mid, pdu.hits, pdu.srcas, 
			                      pdu.dstas, pdu.sasgid, pdu.dasgid, pdu.nexthop, pdu.inif, pdu.outif, pdu.first, pdu.last,  pdu.sport, 
			                      pdu.dport, pdu.tcpflags, pdu.tos, pdu.srcmask, pdu.dstmask, pdu.pktid,  pdu.direct);
		}
	}
	fclose(file_read_p);
	fclose(file_write_p);
	memset(command,0,sizeof(command));
	sprintf(command,"rm -fr %s",file_read);
	system(command);
	memset(file_read,0,sizeof(file_write));
	sprintf(file_read,"%s/%s",file_names[i].tempfilepath,file_names[i].finalfile);
	rename(file_write,file_read);
}
*/
/*
//流量分析-->业务分析-->业务ID
void app_match(void *myparam)
{
	int i=*(int*)myparam;
	char file_read[160]={0};
	char file_write[160]={0};
	char command[128]={0};
	sprintf(command,"/bin/bzcat %s/%s >> %s/z%s",file_names[i].absolutepath,file_names[i].detailfile,file_names[i].tempfilepath,file_names[i].tempfile);
	system(command);
	sprintf(file_read,"%s/z%s",file_names[i].tempfilepath,file_names[i].tempfile);
	sprintf(file_write,"%s/%s",file_names[i].tempfilepath,file_names[i].tempfile);
	FILE * file_read_p=fopen(file_read,"r");
	FILE * file_write_p=fopen(file_write,"w+");
	if(!file_read_p||!file_write_p)
		return ;
	char linebuff[240]={0};
	struct flow pdu;
	int writeflag=8;//减到零时写文件,即保存数据
	while(fgets(linebuff,240,file_read_p))
	{
		memset(&pdu,0,sizeof(pdu));
		sscanf(linebuff,"%c%*[|]%c%*[|]%hu%*[|]%u%*[|]%u%*[|]%hu%*[|]%hu%*[|]%hu%*[|]%hu%*[|]%u%*[|]%u%*[|]%u%*[|]%u%*[|]%u%*[|]%u%*[|]%u%*[|]%hu%*[|]%hu%*[|]%hu%*[|]%c%*[|]%c%*[|]%c%*[|]%c%*[|]%c%*[|]%c%*[|]%c%*[|]%c", 
		                &pdu.rid, &pdu.mid, &pdu.hits, &pdu.saddr, &pdu.daddr, &pdu.srcas, &pdu.dstas, &pdu.sasgid, &pdu.dasgid, &pdu.nexthop, &pdu.inif, &pdu.outif, &pdu.first, &pdu.last,&pdu.pkts, &pdu.octets, &pdu.sport,
		                &pdu.dport, &pdu.port, &pdu.tcpflags, &pdu.proto, &pdu.tos, &pdu.srcmask, &pdu.dstmask, &pdu.pktid, &pdu.portid, &pdu.direct);
		writeflag--;//直接去掉rid
		
		if(file_names[i].mymatch.mid)
		{
			if(file_names[i].mymatch.mid==pdu.mid) writeflag--;
			else 
			{
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
			
		if(file_names[i].mymatch.as)
		{
			if(file_names[i].mymatch.as==pdu.srcas || file_names[i].mymatch.as==pdu.dstas) writeflag--;
			else  
			{
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
			
		if(file_names[i].mymatch.ip)
		{
			if(file_names[i].mymatch.ip==pdu.saddr || file_names[i].mymatch.ip==pdu.daddr) writeflag--;
			else  
			{
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
			
		if(file_names[i].mymatch.gid)
		{
			if(file_names[i].mymatch.gid==pdu.sasgid || file_names[i].mymatch.gid==pdu.dasgid) writeflag--;
			else  
			{
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
			
		writeflag--;//去掉myif
			
		if(file_names[i].mymatch.port)
		{
			if(file_names[i].mymatch.port==pdu.sport || file_names[i].mymatch.port==pdu.dport || file_names[i].mymatch.port==pdu.port) writeflag--;
			else  
			{
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
		
		if(file_names[i].mymatch.proto)
		{
			if(file_names[i].mymatch.proto==pdu.proto ) writeflag--;
			else  
			{
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
		
		if(0==writeflag)
		{	
			fprintf(file_write_p,"%hu|%c|%c|%u|%u|%u|%u|%c|%c|%hu|%u|%u|%hu|%hu|%hu|%hu|%u|%u|%u|%hu|%hu|%c|%c|%c|%c|%c|%c\n",
			                      pdu.port, pdu.portid, pdu.proto,pdu.saddr, pdu.daddr,pdu.octets,pdu.pkts,pdu.rid, pdu.mid, pdu.hits,  pdu.srcas, 
			                      pdu.dstas, pdu.sasgid, pdu.dasgid, pdu.nexthop, pdu.inif, pdu.outif, pdu.first, pdu.last,  pdu.sport, 
			                      pdu.dport, pdu.tcpflags, pdu.tos, pdu.srcmask, pdu.dstmask, pdu.pktid,  pdu.direct);
		}
		writeflag=8;
	}
	fclose(file_read_p);
	fclose(file_write_p);
	memset(command,0,sizeof(command));
	sprintf(command,"rm -fr %s",file_read);
	system(command);
	memset(file_read,0,sizeof(file_write));
	sprintf(file_read,"%s/%s",file_names[i].tempfilepath,file_names[i].finalfile);
	rename(file_write,file_read);
}
*/


void all_match(void *myparam)
{
	int i=*(int*)myparam;
	char file_read[160]={0};
	char file_write[160]={0};
	char command[128]={0};
	sprintf(command,"/usr/bin/bzcat %s/%s >> %s/z%s",file_names[i].absolutepath,file_names[i].detailfile,file_names[i].tempfilepath,file_names[i].tempfile);
	system(command);
	sprintf(file_read,"%s/z%s",file_names[i].tempfilepath,file_names[i].tempfile);
	sprintf(file_write,"%s/%s",file_names[i].tempfilepath,file_names[i].tempfile);
	int file_read_p=open(file_read,O_RDONLY);
	FILE * file_write_p=fopen(file_write,"w+");
	if(-1 == file_read_p||!file_write_p)
		return ;
	char linebuff[240]={0};
	
	//void *mmap(void *start, size_t length, int prot, int flags,int fd, off_t offset);
	int len=lseek(file_read_p,0,SEEK_END);
	int flow_num = len/sizeof(struct flow);
	struct flow *pdu = NULL;
	pdu = (struct flow *)mmap(NULL,len,PROT_READ,MAP_SHARED,file_read_p,0);
	if(!pdu)
		return;
	
	int writeflag=8;//减到零时写文件,即保存数据
	int loop=0;
	for(loop = 0;loop < flow_num; loop++)
	{
		//printf("pdu[loop]..rid=%d\n",pdu[loop]..rid);
		if(file_names[i].mymatch.rid != (u_int8)-1)
		{
			if(file_names[i].mymatch.rid==pdu[loop].rid) writeflag--;
			else
			{
				//printf("writeflag=%d\n",writeflag);
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
		
		if(file_names[i].mymatch.mid != (u_int8)-1 )
		{
			if(file_names[i].mymatch.mid==pdu[loop].mid) 
				writeflag--;
			else 
			{
				//printf("writeflag=%d\n",writeflag);
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
			
		if(file_names[i].mymatch.as != (u_int16)-1 )
		{
			if(file_names[i].mymatch.as==pdu[loop].srcas || file_names[i].mymatch.as==pdu[loop].dstas) writeflag--;
			else  
			{
				//printf("writeflag=%d\n",writeflag);
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
			
//		if(file_names[i].mymatch.ip != (u_int64)-1)
		if(strcmp(temp_ip,"-1") != 0 )
		{
			if(file_names[i].mymatch.ip==pdu[loop].saddr || file_names[i].mymatch.ip==pdu[loop].daddr) writeflag--;
			else  
			{
				//printf("writeflag=%d\n",writeflag);
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
			
		if(file_names[i].mymatch.gid != (u_int16)-1)
		{
			if(file_names[i].mymatch.gid==pdu[loop].sasgid || file_names[i].mymatch.gid==pdu[loop].dasgid) writeflag--;
			else  
			{
				//printf("writeflag=%d\n",writeflag);
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
			
		/*
		if(file_names[i].mymatch.myif != (u_int32)-1)
		{
			if(file_names[i].mymatch.myif==pdu[loop].inif || file_names[i].mymatch.myif==pdu[loop].outif) writeflag--;
			else  
			{
				//printf("writeflag=%d\n",writeflag);
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
		*/
		
		int j=0;
		if(interfaces[j] == (u_int32)-1 )
		{
			writeflag--;
		}
		else
		{
			for(j=0;j<file_names[i].mymatch.myif ;j++)
			{
				if(interfaces[j] == pdu[loop].inif || interfaces[j] == pdu[loop].outif )
				{
					writeflag--;
					goto buhaoxie;
				}
			}
			writeflag=8;
			continue;
		}
		
		buhaoxie:
		
		if(file_names[i].mymatch.port != (u_int16)-1 )
		{
			if(file_names[i].mymatch.port==pdu[loop].sport || file_names[i].mymatch.port==pdu[loop].dport || file_names[i].mymatch.port==pdu[loop].port) writeflag--;
			else  
			{
				//printf("writeflag=%d\n",writeflag);
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
		
		if(file_names[i].mymatch.proto != (u_int8)-1 )
		{
			if(file_names[i].mymatch.proto==pdu[loop].proto ) writeflag--;
			else  
			{
				//printf("writeflag=%d\n",writeflag);
				writeflag=8;
				continue;
			}
		}
		else writeflag--;
		
	//	printf("writeflag=%d\n",writeflag);
		
		if(0==writeflag)
		{	
			fprintf(file_write_p,"%hu|%u|%u|%u|%u|%u|%u|%u|%u|%hu|%u|%u|%hu|%hu|%hu|%hu|%u|%u|%u|%hu|%hu|%u|%u|%u|%u|%u|%u\n",
			                      pdu[loop].port, pdu[loop].portid, pdu[loop].proto,pdu[loop].saddr, pdu[loop].daddr,pdu[loop].octets,pdu[loop].pkts,pdu[loop].rid, pdu[loop].mid, pdu[loop].hits,  pdu[loop].srcas, 
			                      pdu[loop].dstas, pdu[loop].sasgid, pdu[loop].dasgid, pdu[loop].nexthop, pdu[loop].inif, pdu[loop].outif, pdu[loop].first, pdu[loop].last,  pdu[loop].sport, 
			                      pdu[loop].dport, pdu[loop].tcpflags, pdu[loop].tos, pdu[loop].srcmask, pdu[loop].dstmask, pdu[loop].pktid,  pdu[loop].direct);
		}
		writeflag=8;
	}
	close(file_read_p);
	fclose(file_write_p);
	memset(command,0,sizeof(command));
	sprintf(command,"rm -fr %s",file_read);
	system(command);
	memset(file_read,0,sizeof(file_write));
	sprintf(file_read,"%s/%s",file_names[i].tempfilepath,file_names[i].finalfile);
	rename(file_write,file_read);
}

void destroy_threadpool(threadpool destroyme)
{
    _threadpool *pool = (_threadpool *) destroyme;

    // add your code here to kill a threadpool
    int i = 0;

    pthread_mutex_lock( &pool->tp_mutex );

    if( pool->tp_index < pool->tp_total ) {//为什么？
        pthread_cond_wait( &pool->tp_full, &pool->tp_mutex );
    }

    pool->tp_stop = 1;

    for( i = 0; i < pool->tp_index; i++ ) {//依次唤醒每个cond中线程
        _thread * thread = pool->tp_list[ i ];

        pthread_mutex_lock( &thread->mutex );
        pthread_cond_signal( &thread->cond ) ;
        pthread_mutex_unlock ( &thread->mutex );
    }

    if( pool->tp_total > 0 ) {
        //printf( "waiting for %d thread(s) to exit ", pool->tp_total );
        pthread_cond_wait( &pool->tp_empty, &pool->tp_mutex );
    }

    for( i = 0; i < pool->tp_index; i++ ) {
        free( pool->tp_list[ i ] );
        pool->tp_list[ i ] = NULL;
    }

    pthread_mutex_unlock( &pool->tp_mutex );

    pool->tp_index = 0;

    pthread_mutex_destroy( &pool->tp_mutex );
    pthread_cond_destroy( &pool->tp_idle );
    pthread_cond_destroy( &pool->tp_full );
    pthread_cond_destroy( &pool->tp_empty );

    free( pool->tp_list );
    free( pool );
}

int version_info(void)
{
	FILE *fp;
	char content[1024];
	int count = 0;
	
	fp = fopen(VERSION_PATH, "r");
	if (fp == NULL)
	{
		write_log("open version.xml failed");
		return -1;
	}
	
	//get time_interval
	while(fgets(content,256,fp)!=NULL)
	{
		if (strstr(content,"hisDataSaveTime_m") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &hisDataSaveTime_m);  
			count++;
		}
		else if (strstr(content,"hisDataSaveTime_h") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &hisDataSaveTime_h);  
			count++;
		}
		else if (strstr(content,"hisDataSaveTime_d") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &hisDataSaveTime_d);  
			count++;
		}
		else if (strstr(content,"allTrafficSaveTime") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &allTrafficSaveTime);  
			count++;
		}
		else if (strstr(content,"hasdrill_app_router") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &ROUTER_APP_DETAIL);  
			count++;
		}
		else if (strstr(content,"hasdrill_ip_router") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &ROUTER_IP_DETAIL);  
			count++;
		}
		else if (strstr(content,"hasdrill_as_router") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &ROUTER_AS_DETAIL); 
			count++; 
		}
		else if (strstr(content,"hasdrill_app_monitor") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &MONITOR_APP_DETAIL);  
			count++;
		}
		else if (strstr(content,"hasdrill_ip_monitor") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &MONITOR_IP_DETAIL);  
			count++;
		}
		else if (strstr(content,"hasdrill_as_monitor") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &MONITOR_AS_DETAIL); 
			count++; 
		}
		else if (strstr(content,"hasippair") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &HAS_IPPAIR); 
			count++; 
		}
		else if (strstr(content,"hastos") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &HAS_TOS); 
			count++; 
		}
		else if (strstr(content,"hassubnet") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &HAS_SUBNET); 
			count++; 
		}
		else if (strstr(content,"hasaspair") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &HAS_ASPAIR); 
			count++; 
		}
		else if (strstr(content,"hasas") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &HAS_AS);  
			count++;
		}
		else if (strstr(content,"haspkt") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &HAS_PKT);  
			count++;
		}
		else if (strstr(content,"hasnexthop") != NULL)
		{
			sscanf(content, "%*[^>]>%d", &HAS_NEXTHOP); 
			count++; 
		}
	}
	if (count != 17)
	{
		printf("read version.xml failed\n");
		#if 1
		printf("hisDataSaveTime_m=%d\n", hisDataSaveTime_m);
		printf("hisDataSaveTime_h=%d\n", hisDataSaveTime_h);
		printf("hisDataSaveTime_d=%d\n", hisDataSaveTime_d);
		printf("allTrafficSaveTime=%d\n", allTrafficSaveTime);
		printf("ROUTER_APP_DETAIL=%d\n", ROUTER_APP_DETAIL);
		printf("ROUTER_IP_DETAIL=%d\n", ROUTER_IP_DETAIL);
		printf("ROUTER_AS_DETAIL=%d\n", ROUTER_AS_DETAIL);
		printf("MONITOR_APP_DETAIL=%d\n", MONITOR_APP_DETAIL);
		printf("MONITOR_IP_DETAIL=%d\n", MONITOR_IP_DETAIL);
		printf("MONITOR_AS_DETAIL=%d\n", MONITOR_AS_DETAIL);
		printf("HAS_IPPAIR=%d\n", HAS_IPPAIR);
		printf("HAS_TOS=%d\n", HAS_TOS);
		printf("HAS_SUBNET=%d\n", HAS_SUBNET);
		printf("HAS_ASPAIR=%d\n", HAS_ASPAIR);
		printf("HAS_AS=%d\n", HAS_AS);
		printf("HAS_PKT=%d\n", HAS_PKT);
		printf("HAS_NEXTHOP=%d\n", HAS_NEXTHOP);
	#endif
		exit(0);
	}
	
	fclose(fp);

	return 0;
}

//线程超过最大值会失败返回-1；成功返回0，insert_thread_into_threadpool
int save_thread( _threadpool * pool, _thread * thread )
{
    int ret = -1;

    pthread_mutex_lock( &pool->tp_mutex );

    if( pool->tp_index < pool->tp_max_index ) {
        pool->tp_list[ pool->tp_index ] = thread;
        pool->tp_index++;
        ret = 0;

        pthread_cond_signal( &pool->tp_idle );//释放tp_idle中的一个线程

        if( pool->tp_index >= pool->tp_total ) {
            pthread_cond_signal( &pool->tp_full );
        }
    }

    pthread_mutex_unlock( &pool->tp_mutex );

    return ret;
}

void * wrapper_fn( void * arg )//用作创建线程的处理函数
{
    _thread * thread = (_thread*)arg;
    _threadpool * pool = (_threadpool*)thread->parent;

    for( ; 0 == ((_threadpool*)thread->parent)->tp_stop; ) 
    {
        thread->fn( thread->arg );

        pthread_mutex_lock( &thread->mutex );
        if( 0 == save_thread( thread->parent, thread ) ) 
        {
            pthread_cond_wait( &thread->cond, &thread->mutex );//进入条件变量cond，释放锁mutex
            pthread_mutex_unlock( &thread->mutex );
        } else //线程超过最大值
        	{
            pthread_mutex_unlock( &thread->mutex );
            pthread_cond_destroy( &thread->cond );
            pthread_mutex_destroy( &thread->mutex );

            free( thread );
            break;
        	}
    }

    pthread_mutex_lock( &pool->tp_mutex );
    pool->tp_total--;
    if( pool->tp_total <= 0 ) pthread_cond_signal( &pool->tp_empty );//
    pthread_mutex_unlock( &pool->tp_mutex );

    return NULL;
}

int dispatch_threadpool(threadpool from_me, dispatch_fn dispatch_to_here, char *arg)
{
    int ret = 0;

    _threadpool *pool = (_threadpool *) from_me;
    pthread_attr_t attr;
    _thread * thread = NULL;

    // add your code here to dispatch a thread
    pthread_mutex_lock( &pool->tp_mutex );

    if( pool->tp_index <= 0 && pool->tp_total >= pool->tp_max_index ) {
        pthread_cond_wait( &pool->tp_idle, &pool->tp_mutex );
    }

    if( pool->tp_index <= 0 ) {
        _thread * thread = ( _thread * )malloc( sizeof( _thread ) );
        thread->id = 0;
        pthread_mutex_init( &thread->mutex, NULL );
        pthread_cond_init( &thread->cond, NULL );
        thread->fn = dispatch_to_here;
        strncpy(thread->arg, arg,4);
        thread->parent = pool;

        pthread_attr_init( &attr );
        pthread_attr_setdetachstate( &attr,PTHREAD_CREATE_DETACHED );

        if( 0 == pthread_create( &thread->id, &attr, wrapper_fn, thread ) ) {
            pool->tp_total++;
            //printf( "create thread#%ld \n", thread->id );
        }
        else
        {
            ret = -1;
            printf( "cannot create thread \n" );
            pthread_mutex_destroy( &thread->mutex );
            pthread_cond_destroy( &thread->cond );
            free( thread );
        }
    }
    else
    {
        pool->tp_index--;//save_thread()中此量在增加
        thread = pool->tp_list[ pool->tp_index ];
        pool->tp_list[ pool->tp_index ] = NULL;

        thread->fn = dispatch_to_here;
        strcpy(thread->arg, arg);
        thread->parent = pool;

        pthread_mutex_lock( &thread->mutex );
        pthread_cond_signal( &thread->cond ) ;//唤醒cond中的一个线程
        pthread_mutex_unlock ( &thread->mutex );
    }

    pthread_mutex_unlock( &pool->tp_mutex );

    return ret;
}

threadpool create_threadpool(int num_threads_in_pool)
{
    _threadpool *pool;

    if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
        return NULL;

    pool = (_threadpool *) malloc(sizeof(_threadpool));
    if (pool == NULL) {
        fprintf(stderr, "Out of memory creating a new threadpool! ");
        return NULL;
    }

    // add your code here to initialize the newly created threadpool
    pthread_mutex_init( &pool->tp_mutex, NULL );
    pthread_cond_init( &pool->tp_idle, NULL );
    pthread_cond_init( &pool->tp_full, NULL );
    pthread_cond_init( &pool->tp_empty, NULL );
    pool->tp_max_index = num_threads_in_pool;
    pool->tp_index = 0;
    pool->tp_stop = 0;
    pool->tp_total = 0;
    pool->tp_list = ( _thread ** )malloc( sizeof( void * ) * MAXT_IN_POOL );
    memset( pool->tp_list, 0, sizeof( void * ) * MAXT_IN_POOL );

    return (threadpool) pool;
}

int pthread_pool_insert_file(int filecount,int interface_flag) {
  threadpool tp;
  tp = create_threadpool(THREADPOOL_NUM);
  int order[MAX_FILE_COUNT];
  int i;
  for(i=0;i<filecount;i++)
  {
  	order[i]=i;
  }
  int *p=order;
  
  for(i=0;i<filecount;i++)
	{
		dispatch_threadpool(tp,all_match,(void *)p++);
	}
  /*
  if(interface_flag)
  {
		for(i=0;i<filecount;i++)
		{
			dispatch_threadpool(tp,interface_match,(void *)p++);
		}
	}
	else
	{
		for(i=0;i<filecount;i++)
		{
			dispatch_threadpool(tp,app_match,(void *)p++);
		}
	}
	*/
	
	destroy_threadpool( tp );

  return 0;
}

int get_file_count(struct tm *tm_start,struct tm *tm_end)
{
	time_t time_start,time_end,time_now=time(NULL),earliest_time;
	struct tm tm_now;
	localtime_r(&time_now,&tm_now);
	earliest_time=time_now-(DETAILFILE_SAVETIME-1)*24*3600-tm_now.tm_hour*3600-tm_now.tm_min*60-tm_now.tm_sec;
	time_start=mktime(tm_start);
	time_start=(time_start > earliest_time ) ? time_start : earliest_time;
	time_end=mktime(tm_end);
	if(time_end<=time_start)
		return 0;
	localtime_r(&time_start,tm_start);
	
	tm_start->tm_min=(tm_start->tm_min/DETAIL_SEPARATE_TIME)*DETAIL_SEPARATE_TIME;
	tm_start->tm_sec=0;
	tm_end->tm_min=(tm_end->tm_min/DETAIL_SEPARATE_TIME)*DETAIL_SEPARATE_TIME;
	tm_end->tm_sec=0;
	
	return (int)difftime(mktime(tm_end),mktime(tm_start))/(DETAIL_SEPARATE_TIME*60)+1;
}

void get_detail_file_name(int filecount,match_parameter file[],struct tm *tm_start,struct tm *tm_end,match_st mymatch_param)
{
	match_parameter* param=file_names;
	if(NULL==param) 
	{
		printf("内存分配失败");
		return ;
	}
	memset(file_names,0,sizeof(file_names));
	time_t time_start,time_end=mktime(tm_end);
	struct tm *temp_tmtime=(struct tm *)calloc(1,sizeof(struct tm));
	if(NULL==temp_tmtime) 
	{
		printf("内存分配失败");
		return;
	}
	//strncpy((char*)temp_tmtime,(char *)tm_start,sizeof(struct tm));
	*temp_tmtime=*tm_start;
	time_t temp_time;
	int i;
	if ((u_int8)-1 == mymatch_param.mid)
	{
		for(i=0;i<filecount;i++)
		{
			param[i].mymatch=mymatch_param;
			sprintf( param[i].absolutepath,"%s/%04d%02d%02d/R%d",DETAILFILEPATH,temp_tmtime->tm_year+1900,temp_tmtime->tm_mon+1,temp_tmtime->tm_mday,mymatch_param.rid%8);
		//	printf("param[i].absolutepath=%s\n",param[i].absolutepath);
			sprintf( param[i].detailfile,"%02d%02d.bz2",temp_tmtime->tm_hour,temp_tmtime->tm_min);
			sprintf( param[i].tempfilepath,"%s/%s",MATCHEDFILEPATH,temp_dir);
			sprintf( param[i].tempfile,"begin_R%04d%02d%02d%02d%02d.txt",temp_tmtime->tm_year+1900,temp_tmtime->tm_mon+1,temp_tmtime->tm_mday,temp_tmtime->tm_hour,temp_tmtime->tm_min);
			sprintf( param[i].finalfile,"end_R%04d%02d%02d%02d%02d.txt",temp_tmtime->tm_year+1900,temp_tmtime->tm_mon+1,temp_tmtime->tm_mday,temp_tmtime->tm_hour,temp_tmtime->tm_min);
			temp_time=mktime(temp_tmtime);
			temp_time+=60*DETAIL_SEPARATE_TIME;
			localtime_r(&temp_time,temp_tmtime);
		}
	}
	else
	{
		for(i=0;i<filecount;i++)
		{
			param[i].mymatch=mymatch_param;
			
			sprintf( param[i].absolutepath,"%s/%04d%02d%02d/M%d",DETAILFILEPATH,temp_tmtime->tm_year+1900,temp_tmtime->tm_mon+1,temp_tmtime->tm_mday,mymatch_param.mid%8);
		//	printf("param[i].absolutepath=%s\n",param[i].absolutepath);
			sprintf( param[i].detailfile,"%02d%02d.bz2",temp_tmtime->tm_hour,temp_tmtime->tm_min);
			sprintf( param[i].tempfilepath,"%s/%s",MATCHEDFILEPATH,temp_dir);
			sprintf( param[i].tempfile,"begin_M%04d%02d%02d%02d%02d.txt",temp_tmtime->tm_year+1900,temp_tmtime->tm_mon+1,temp_tmtime->tm_mday,temp_tmtime->tm_hour,temp_tmtime->tm_min);
			sprintf( param[i].finalfile,"end_M%04d%02d%02d%02d%02d.txt",temp_tmtime->tm_year+1900,temp_tmtime->tm_mon+1,temp_tmtime->tm_mday,temp_tmtime->tm_hour,temp_tmtime->tm_min);
			temp_time=mktime(temp_tmtime);
			temp_time+=60*DETAIL_SEPARATE_TIME;
			localtime_r(&temp_time,temp_tmtime);
		}
	}
	free(temp_tmtime);
	return;
}

int main(int argc,char **argv)
{
	if( argc != 12 )
	{
		write_log("parameter count failed");
		return -1;
	}
	//daemon_init();
	char detailpostpath[64]={0};
	sprintf(detailpostpath,"/data/detailtemp/detailpost/%s",argv[11]);
	if(access(detailpostpath,F_OK)==0)
	{
		return 0;
	}
	
	int reValue = version_info();//此函数给许多全局变量赋值
	
	if (reValue < 0)
	{
		return 0;
	}
	struct tm tm_start,tm_end;
	//时间测试
	time_t timestart,timeend,tm_now1;
	timestart=atoi(argv[9]);
	localtime_r(&timestart,&tm_start);
	time(&tm_now1);
	timeend=atoi(argv[10]);
	timeend= timeend<tm_now1?timeend:tm_now1;
	//timeend = timestart - 600;
	localtime_r(&timeend,&tm_end);
	
	
	
	int filecount = get_file_count(&tm_start,&tm_end);

	match_st mymatch_param={0};//非查看接口详细，接口置-1
	//参数测试,没有用到参数都传-1
	
	
	mymatch_param.rid=atoi(argv[1]);
	mymatch_param.mid=atoi(argv[2]);
	mymatch_param.ip=strtoul(argv[3],NULL,10);
	memset(temp_ip,0,sizeof(temp_ip));
	sprintf(temp_ip,"%s",argv[3]);
	mymatch_param.as=atoi(argv[4]);
	mymatch_param.gid=atoi(argv[5]);
	//mymatch_param.myif=atoi(argv[6]);
	mymatch_param.port=atoi(argv[7]);
	mymatch_param.proto=atoi(argv[8]);
	
	memset(temp_dir,0,sizeof(temp_dir));
	sprintf(temp_dir,"%s",argv[11]);
	
	memset(interfaces,0,sizeof(interfaces));
	int i=0;
	char in_list[64]={0};
	char *p=in_list;
	sprintf(in_list,"%s",argv[6]);
	while(*p && sscanf(p,"%u",&interfaces[i]))
	{
		while(*p != ',' && *p)
		{
			p++;
		}
		i++;
		p++;
	}
	mymatch_param.myif=i;
	
	
	
	
	/*if(mymatch_param.mid != (u_int8)-1 )
	{
		memset(dir_prefix,0,8);
		sprintf(dir_prefix,"M_");
	}
	else
	{
		memset(dir_prefix,0,8);
		sprintf(dir_prefix,"R_");
	}*/
	

	//建立临时数据文件夹
	char command[256]={0};
	sprintf(command,"mkdir -p %s/%s;mkdir -p /data/detailtemp/detailpost",MATCHEDFILEPATH,argv[11]);
	system(command);
	

	get_detail_file_name(filecount,file_names,&tm_start,&tm_end,mymatch_param);
	
	//这里调用数据分析程序
	
	char analysis_type[24];
	
//	if( mymatch_param.ip != (u_int32)-1 )
	if(strcmp(temp_ip,"-1") != 0)
	{
		memset(analysis_type,0,24);
		sprintf(analysis_type,"ip  %llu" ,mymatch_param.ip);
	}
	else if( mymatch_param.port != (u_int16)-1 && mymatch_param.proto != (u_int8)-1 )
	{
		memset(analysis_type,0,24);
		sprintf(analysis_type,"app");
	}
	else if( mymatch_param.port != (u_int16)-1 )
	{
		memset(analysis_type,0,24);
		sprintf(analysis_type,"proto");
	}
	else
	{
		
	}
	
	memset(command,0,sizeof(command));
	sprintf(command,"/dhcc/forceview/src/detail/flow-detail %s %s",argv[11],analysis_type);
//	FILE *file123=fopen("/root/nimeimei.txt","a+");
//	fprintf(file123,"%s",command);
//	fclose(file123);
	if( fork() == 0  )
	{
		system(command);
	}
	else
	{
	
	
		pthread_pool_insert_file(filecount-1,mymatch_param.myif);
	
		memset(command,0,sizeof(command));
		sprintf(command,"cd %s/%s/;touch allend_file.txt",MATCHEDFILEPATH,argv[11]);
		system(command);
	
		return 0;
	
	}
}

int write_log(char *log_msg)
{
	time_t time1;
	struct tm m_tm;
	char time_str[128],filename[128];
	FILE *f1;
	
	time(&time1);
	localtime_r(&time1, &m_tm);
	sprintf(time_str, "%04d-%02d-%02d %02d:%02d:00", m_tm.tm_year+1900, m_tm.tm_mon+1, m_tm.tm_mday, m_tm.tm_hour, m_tm.tm_min);
	
	sprintf(filename, "/data/log/delete.log");
	f1 = fopen(filename, "a+");
	if (f1 == NULL)
	{
		return -1;
	}
	
	fprintf(f1, "%s   %s\n", time_str, log_msg);
	
	fclose(f1);
	return 0;
}

int daemon_init(void)
{
	pid_t pid;
	int ret;

	if ((pid = fork()) < 0)
	{
		write_log("daemon_init:fork function failed\n");
		return (-1);
	}
	else if (pid != 0)
	{	
		exit(0); /* parent goes bye-bye */
	}
	if ((ret=setsid()) < 0) /* become session leader */
	{
		write_log("daemon_init:unable to setsid \n");
		if (DEBUG)
		{
			printf("unable to setsid.\n");
		}
	}

	setpgrp();
	return(0);
}
