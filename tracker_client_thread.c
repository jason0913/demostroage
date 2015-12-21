#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/vfs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tracker_client_thread.h"
#include "shared_func.h"
#include "tracker_types.h"
#include "storage_global.h"
#include "logger.h"
#include "fdfs_global.h"
#include "sockopt.h"
#include "tracker_proto.h"
#include "storage_func.h"

static pthread_mutex_t reporter_thread_lock;

int tracker_report_init()
{

	int result;
	if (0 !=(result =init_pthread_lock(&reporter_thread_lock)))
	{
		return result;
	}

#ifdef __DEBUG__
	printf("tracker_report_init done! file:%s,line:%d\n",__FILE__,__LINE__);
#endif

	return 0;
}

int tracker_report_destroy()
{
	printf("tracker_report_destroy done!\n");
	return 0;
}

static int tracker_check_response(TrackerServerInfo *pTrackerServer)
{

#ifdef __DEBUG__
	printf("tracker_check_response to be finish ------>>>>> file:%s,line:%d\n",__FILE__,__LINE__);
#endif

	return 0;
}

int tracker_report_join(TrackerServerInfo *pTrackerServer)
{
	char out_buff[sizeof(TrackerHeader)+sizeof(TrackerStorageJoinBody)];
	TrackerHeader *pHeader;
	TrackerStorageJoinBody *pReqBody;

	pHeader = (TrackerHeader *)out_buff;
	pReqBody = (TrackerStorageJoinBody *)(out_buff+sizeof(TrackerHeader));

	memset(out_buff,0,sizeof(out_buff));
	sprintf(pHeader->pkg_len,"%x", sizeof(TrackerStorageJoinBody));
	pHeader->cmd = TRACKER_PROTO_CMD_STORAGE_JOIN;
	strcpy(pReqBody->group_name, g_group_name);
	sprintf(pReqBody->storage_port, "%x", g_server_port);

	if (1 != tcpsenddata(pTrackerServer->sock, out_buff, sizeof(out_buff), \
				g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"tracker server %s:%d, send data fail, " \
			"errno: %d, error info: %s.", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, \
			errno, strerror(errno));

		return errno != 0 ? errno : EPIPE;
	}

	return tracker_check_response(pTrackerServer);
}

static int tracker_report_stat(TrackerServerInfo *pTrackerServer)
{
	char out_buff[sizeof(TrackerHeader) + sizeof(TrackerStatReportReqBody)];
	TrackerHeader *pHeader;
	TrackerStatReportReqBody *pStatBuff;
	struct statfs sbuf;

	if (0 != statfs(g_base_path,&sbuf))
	{
		logError("file: %s, line: %d, " \
			"call statfs fail, errno: %d, error info: %s.", \
			__FILE__,__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EACCES;
	}

	pHeader =(TrackerHeader *)out_buff;
	pStatBuff = (TrackerStatReportReqBody *) (out_buff + sizeof(TrackerHeader));
	sprintf(pHeader->pkg_len, "%x", sizeof(TrackerStatReportReqBody));
	pHeader->cmd = TRACKER_PROTO_CMD_STORAGE_REPORT;
	pHeader->status =0;

	int2buff((int)(((double)(sbuf.f_blocks) * sbuf.f_bsize) / FDFS_ONE_MB),\
		pStatBuff->sz_total_mb);
	int2buff((int)(((double)(sbuf.f_bavail) * sbuf.f_bsize) / FDFS_ONE_MB),\
		pStatBuff->sz_free_mb);

	if (1 != tcpsenddata(pTrackerServer->sock,out_buff,\
			sizeof(out_buff),g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"tracker server %s:%d, send data fail, " \
			"errno: %d, error info: %s.", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, \
			errno, strerror(errno));

		return errno != 0 ? errno : EPIPE;
	}

	return tracker_check_response(pTrackerServer);
}

static int tracker_heart_beat(TrackerServerInfo *pTrackerServer, \
			int *pstat_chg_sync_count)
{
	char out_buff[sizeof(TrackerHeader) + sizeof(FDFSStorageStatBuff)];
	TrackerHeader *pHeader;
	FDFSStorageStatBuff *pStatBuff;
	int body_len;

	pHeader = (TrackerHeader *)out_buff;
	if (*pstat_chg_sync_count != g_stat_change_count)
	{
		pStatBuff = (FDFSStorageStatBuff *)( \
				out_buff + sizeof(TrackerHeader));
		int2buff(g_storage_stat.total_upload_count, \
			pStatBuff->sz_total_upload_count);
		int2buff(g_storage_stat.success_upload_count, \
			pStatBuff->sz_success_upload_count);
		int2buff(g_storage_stat.total_download_count, \
			pStatBuff->sz_total_download_count);
		int2buff(g_storage_stat.success_download_count, \
			pStatBuff->sz_success_download_count);
		int2buff(g_storage_stat.total_set_meta_count, \
			pStatBuff->sz_total_set_meta_count);
		int2buff(g_storage_stat.success_set_meta_count, \
			pStatBuff->sz_success_set_meta_count);
		int2buff(g_storage_stat.total_delete_count, \
			pStatBuff->sz_total_delete_count);
		int2buff(g_storage_stat.success_delete_count, \
			pStatBuff->sz_success_delete_count);
		int2buff(g_storage_stat.total_get_meta_count, \
			pStatBuff->sz_total_get_meta_count);
		int2buff(g_storage_stat.success_get_meta_count, \
		 	pStatBuff->sz_success_get_meta_count);
		int2buff(g_storage_stat.last_source_update, \
			pStatBuff->sz_last_source_update);
		int2buff(g_storage_stat.last_sync_update, \
			pStatBuff->sz_last_sync_update);

		*pstat_chg_sync_count = g_stat_change_count;
		body_len = sizeof(FDFSStorageStatBuff);
	}
	else
		body_len =0;

	sprintf(pHeader->pkg_len,"%x",body_len);
	pHeader->cmd = TRACKER_PROTO_CMD_STORAGE_BEAT;
	pHeader->status =0;

	if (1 != tcpsenddata(pTrackerServer->sock,out_buff,\
			sizeof(TrackerHeader) + body_len, g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"tracker server %s:%d, send data fail, " \
			"errno: %d, error info: %s.", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, \
			errno, strerror(errno));
		return errno != 0 ? errno : EPIPE;
	}

	return tracker_check_response(pTrackerServer);
}

static int tracker_sync_notify(TrackerServerInfo *pTrackerServer)
{
	char out_buff[sizeof(TrackerHeader)+sizeof(TrackerStorageSyncReqBody)];
	TrackerHeader *pHeader;
	TrackerStorageSyncReqBody *pReqBody;

	pHeader = (TrackerHeader *) out_buff;
	pReqBody = (TrackerStorageSyncReqBody*)(out_buff+sizeof(TrackerHeader));

	memset(out_buff,0,sizeof(out_buff));
	sprintf(pHeader->pkg_len,"%x",sizeof(TrackerStorageSyncReqBody));
	pHeader->cmd = TRACKER_PROTO_CMD_STORAGE_SYNC_NOTIFY;
	strcpy(pReqBody->src_ip_addr, g_sync_src_ip_addr);
	sprintf(pReqBody->until_timestamp, "%x", g_sync_until_timestamp);

	if (1 != tcpsenddata(pTrackerServer->sock,out_buff,sizeof(out_buff),
		g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"tracker server %s:%d, send data fail, " \
			"errno: %d, error info: %s.", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, \
			errno, strerror(errno));
		return errno != 0 ? errno : EPIPE;
	}
	return tracker_check_response(pTrackerServer);
}

static int tracker_sync_dest_req(TrackerServerInfo *pTrackerServer)
{
	TrackerHeader header;
	TrackerStorageSyncReqBody syncReqbody;
	char *pBuff;
	int in_bytes;
	int result;

	memset(&header,0,sizeof(header));
	header.pkg_len[0] = '\0';
	header.cmd = TRACKER_PROTO_CMD_STORAGE_SYNC_DEST_REQ;

	if (1 != tcpsenddata(pTrackerServer->sock,&header,sizeof(header),\
		g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"tracker server %s:%d, send data fail, " \
			"errno: %d, error info: %s.", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, \
			errno, strerror(errno));

		return errno != 0 ? errno : EPIPE;
	}

	pBuff = (char*)&syncReqbody;
	if (0 != (result=tracker_recv_response(pTrackerServer, \
                &pBuff, sizeof(syncReqbody), &in_bytes)))
	{
		return result;
	}

	if (0 == in_bytes)
	{
		return result;
	}

	if (in_bytes != sizeof(syncReqbody))
	{
		logError("file: %s, line: %d, " \
			"tracker server %s:%d, " \
			"recv body length: %d is invalid", \
			"expect body length: %d", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, in_bytes, \
			sizeof(syncReqbody));
		return EINVAL;
	}

	memcpy(g_sync_src_ip_addr, syncReqbody.src_ip_addr, FDFS_IPADDR_SIZE);
	g_sync_src_ip_addr[FDFS_IPADDR_SIZE-1] = '\0';

	syncReqbody.until_timestamp[TRACKER_PROTO_PKG_LEN_SIZE-1] = '\0';
	g_sync_until_timestamp = strtol(syncReqbody.until_timestamp, NULL, 16);

	memset(&header,0,sizeof(header));
	header.pkg_len[0] ='0';
	header.cmd = TRACKER_PROTO_CMD_STORAGE_RESP;

	if (1 != tcpsenddata(pTrackerServer->sock,&header,sizeof(header),\
		g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"tracker server %s:%d, send data fail, " \
			"errno: %d, error info: %s.", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, \
			errno, strerror(errno));
		return errno != 0 ? errno : EPIPE;
	}

	return 0;
}

static void* tracker_report_thread_entrance(void* arg)
{

	TrackerServerInfo *pTrackerServer;
	char tracker_client_ip[FDFS_IPADDR_SIZE];
	bool sync_old_done;
	int stat_chg_sync_count;
	int sleep_secs;
	time_t current_time;
	time_t last_report_time;
	time_t last_beat_time;

	stat_chg_sync_count =0;

	pTrackerServer = (TrackerServerInfo *) arg;
	pTrackerServer->sock = -1;

	sync_old_done = g_sync_old_done;

	while(g_continue_flag &&  \
		g_tracker_reporter_count < g_tracker_server_count)
	{
		sleep(1);
#ifdef __DEBUG__
	printf("waiting for all thread started file:%s,line:%d\n",__FILE__,__LINE__);
#endif
	}

	sleep_secs = g_heart_beat_interval < g_stat_report_interval ? \
			g_heart_beat_interval : g_stat_report_interval;

	while(g_continue_flag)
	{
		if (pTrackerServer->sock >=0)
		{
			close(pTrackerServer->sock);
		}

		pTrackerServer->sock = socket(AF_INET,SOCK_STREAM,0);
		if (pTrackerServer->sock < 0)
		{
			logError("file: %s, line: %d, " \
				"socket create failed, errno: %d, " \
				"error info: %s.", \
				__FILE__,__LINE__, errno, strerror(errno));
			g_continue_flag = false;
			break;
		}

		if (1 != connectserverbyip(pTrackerServer->sock, \
			pTrackerServer->ip_addr, \
			pTrackerServer->port))
		{
			sleep(g_heart_beat_interval);
			continue;
		}

		getSockIpaddr(pTrackerServer->sock, \
				tracker_client_ip, FDFS_IPADDR_SIZE);
		insert_into_local_host_ip(tracker_client_ip);

		if (0 != tracker_report_join(pTrackerServer))
		{
			sleep(g_heart_beat_interval);
			continue;
		}

		if (! sync_old_done)
		{
			if (0 != pthread_mutex_lock(&reporter_thread_lock))
			{
				logError("file: %s, line: %d, " \
					"call pthread_mutex_lock fail, " \
					"errno: %d, error info:%s.", \
					__FILE__,__LINE__, errno, strerror(errno));

				tracker_quit(pTrackerServer);
				sleep(g_heart_beat_interval);
				continue;
			}

			if (!g_sync_old_done)
			{
				if (0 == tracker_sync_dest_req(pTrackerServer))
				{
					g_sync_old_done = true;
					if (0  != storage_write_to_sync_ini_file())
					{
						g_continue_flag = false;
						pthread_mutex_unlock(&reporter_thread_lock);
						break;
					}
				}
				else
				{
					pthread_mutex_unlock(&reporter_thread_lock);
					tracker_quit(pTrackerServer);
					sleep(g_heart_beat_interval);
					continue;
				}
			}

			if (0 != pthread_mutex_unlock(&reporter_thread_lock))
			{
				logError("file: %s, line: %d, " \
					"call pthread_mutex_unlock fail, " \
					"errno: %d, error info:%s.", \
					__FILE__,__LINE__, errno, strerror(errno));
			}
			sync_old_done = true;
		}
		if (*g_sync_src_ip_addr != '\0' && \
			tracker_sync_notify(pTrackerServer) != 0)
		{
			tracker_quit(pTrackerServer);
			sleep(g_heart_beat_interval);
			continue;
		}

		last_report_time =0;
		last_beat_time = 0;

		while(g_continue_flag)
		{
			current_time = time(NULL);
			if (current_time - last_beat_time >= \
					g_heart_beat_interval)
			{
				if (0 != tracker_heart_beat(pTrackerServer, \
					&stat_chg_sync_count))
				{
					break;
				}

				last_beat_time = current_time;
			}

			if (current_time - last_report_time >= \
					g_stat_report_interval)
			{
				if (0 != tracker_report_stat(pTrackerServer))
				{
					break;
				}
				last_report_time = current_time;
			}
			sleep(sleep_secs);
		}
		if ((!g_continue_flag) && tracker_quit(pTrackerServer) != 0)
		{
		}

		close(pTrackerServer->sock);
		pTrackerServer->sock = -1;
		if (g_continue_flag)
		{
			sleep(sleep_secs);
		}
	}

	if (0 != pthread_mutex_lock(&reporter_thread_lock))
	{
		logError("file: %s, line: %d, " \
			"call pthread_mutex_lock fail, " \
			"errno: %d, error info:%s.", \
			__FILE__,__LINE__, errno, strerror(errno));
	}


	g_tracker_reporter_count++;

#ifdef __DEBUG__
	printf("g_tracker_reporter_count:%d,file:%s,line:%d\n",\
			g_tracker_reporter_count,__FILE__,__LINE__);
#endif

	if (0 != pthread_mutex_unlock(&reporter_thread_lock))
	{
		logError("file: %s, line: %d, " \
			"call pthread_mutex_unlock fail, " \
			"errno: %d, error info:%s.", \
			__FILE__,__LINE__, errno, strerror(errno));
	}

#ifdef __DEBUG__
	printf("tracker_report_thread_entrance done! file:%s,line:%d\n",__FILE__,__LINE__);
#endif
	return NULL;
}

int tracker_report_thread_start()
{

	TrackerServerInfo *pTrackerServer;
	TrackerServerInfo *pServerEnd;
	pthread_attr_t pattr;
	pthread_t tid;

	pthread_attr_init(&pattr);
	pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);

	pServerEnd = g_tracker_server + g_tracker_server_count;
	for (pTrackerServer = g_tracker_server; pTrackerServer < pServerEnd; pTrackerServer++)
	{
		if (0 != pthread_create(&tid,&pattr,tracker_report_thread_entrance,pTrackerServer))
		{
			logError("file: %s, line: %d, " \
				"create thread failed, errno: %d, " \
				"error info: %s.", \
				__FILE__,__LINE__, errno, strerror(errno));

			return errno != 0 ? errno : EAGAIN;
		}

		if (0 != pthread_mutex_lock(&reporter_thread_lock))
		{
			logError("file: %s, line: %d, " \
				"pthread_mutex_lock failed, errno: %d, " \
				"error info: %s.", \
				__FILE__,__LINE__, errno, strerror(errno));

		}

		g_tracker_server_count++;

		if (0 != pthread_mutex_unlock(&reporter_thread_lock))
		{
			logError("file: %s, line: %d, " \
				"pthread_mutex_unlock failed, errno: %d, " \
				"error info: %s.", \
				__FILE__,__LINE__, errno, strerror(errno));

		}
	}
	pthread_attr_destroy(&pattr);

#ifdef __DEBUG__
	printf("tracker_report_thread_start done! file:%s,line:%d\n",__FILE__,__LINE__);
#endif
	return 0;
}