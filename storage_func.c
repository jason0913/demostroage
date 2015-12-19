#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "storage_func.h"
#include "logger.h"
#include "fdfs_global.h"
#include "shared_func.h"
#include "ini_file_reader.h"
#include "tracker_proto.h"
#include "sockopt.h"

#include "client_global.h"

#define DATA_DIR_INITED_FILENAME	".data_init_flag"
#define STORAGE_STAT_FILENAME		"storage_stat.dat"
#define DATA_DIR_COUNT_PER_PATH		16

#define INIT_ITEM_STORAGE_JOIN_TIME	"storage_join_time"
#define INIT_ITEM_SYNC_OLD_DONE		"sync_old_done"
#define INIT_ITEM_SYNC_SRC_SERVER	"sync_src_server"
#define INIT_ITEM_SYNC_UNTIL_TIMESTAMP	"sync_until_timestamp"

#define STAT_ITEM_TOTAL_UPLOAD		"total_upload_count"
#define STAT_ITEM_SUCCESS_UPLOAD	"success_upload_count"
#define STAT_ITEM_TOTAL_DOWNLOAD	"total_download_count"
#define STAT_ITEM_SUCCESS_DOWNLOAD	"success_download_count"
#define STAT_ITEM_LAST_SOURCE_UPD	"last_source_update"
#define STAT_ITEM_LAST_SYNC_UPD		"last_sync_update"
#define STAT_ITEM_TOTAL_SET_META	"total_set_meta_count"
#define STAT_ITEM_SUCCESS_SET_META	"success_set_meta_count"
#define STAT_ITEM_TOTAL_DELETE		"total_delete_count"
#define STAT_ITEM_SUCCESS_DELETE	"success_delete_count"
#define STAT_ITEM_TOTAL_GET_META	"total_get_meta_count"
#define STAT_ITEM_SUCCESS_GET_META	"success_get_meta_count"

static int storage_stat_fd = -1;

static int storage_cmp_by_ip_and_port(const void *p1, const void *p2)
{
	int res;

	res = strcmp(((TrackerServerInfo *)p1)->ip_addr,
			((TrackerServerInfo *)p2)->ip_addr);
	if (0 != res)
	{
		return res;
	}
	return ((TrackerServerInfo *)p1)->port - ((TrackerServerInfo*)p2)->port;
}

static void insert_into_sorted_servers(TrackerServerInfo *pInsertedServer)
{
	TrackerServerInfo *pDestServer;
	for (pDestServer=g_tracker_server+g_tracker_server_count; \
		pDestServer>g_tracker_server; pDestServer--)
	{
		if (storage_cmp_by_ip_and_port(pInsertedServer, \
			pDestServer-1) > 0)
		{
			memcpy(pDestServer, pInsertedServer, \
				sizeof(TrackerServerInfo));
			return;
		}

		memcpy(pDestServer, pDestServer-1, sizeof(TrackerServerInfo));
	}

	memcpy(pDestServer, pInsertedServer, sizeof(TrackerServerInfo));
}

static int copy_tracker_servers(const char *filename, char **ppTrackerServers)
{

	char **ppSrc;
	char **ppEnd;
	TrackerServerInfo destServer;
	char *pSeperator;
	char szHost[128];
	int nHostLen;

	memset(&destServer,0,sizeof(TrackerServerInfo));
	ppEnd = ppTrackerServers + g_tracker_server_count;

	g_tracker_server_count =0;
	for (ppSrc = ppTrackerServers; ppSrc < ppEnd; ppSrc++)
	{
		if (NULL == (pSeperator = strchr(*ppSrc,':')))
		{
			logError("file: %s, line: %d, " \
				"conf file \"%s\", " \
				"tracker_server \"%s\" is invalid, " \
				"correct format is host:port", \
				__FILE__,__LINE__, filename, *ppSrc);
			return EINVAL;
		}

		nHostLen = pSeperator -(*ppSrc);
		if (nHostLen >= sizeof(szHost))
		{
			nHostLen = sizeof(szHost) -1;
		}
		memcpy(szHost,*ppSrc,nHostLen);
		szHost[nHostLen] ='\0';

		if (INADDR_NONE == getIpaddrByName(szHost,destServer.ip_addr,sizeof(destServer.ip_addr)))
		{
			logError("file: %s, line: %d, " \
				"conf file \"%s\", " \
				"host \"%s\" is invalid, ",\
				__FILE__,__LINE__, filename, szHost);
			return EINVAL;
		}

		if (0 == strcmp(destServer.ip_addr,"127.0.0.1"))
		{
			logError("file: %s, line: %d, " \
				"conf file \"%s\", " \
				"host \"%s\" is invalid, can not localhost",\
				__FILE__,__LINE__, filename, szHost);
			return EINVAL;
		}

		destServer.port = atoi(pSeperator+1);
		if (destServer.port <=0)
		{
			destServer.port = FDFS_TRACKER_SERVER_DEF_PORT;
		}

		if (bsearch(&destServer, g_tracker_server, \
			g_tracker_server_count, \
			sizeof(TrackerServerInfo), \
			storage_cmp_by_ip_and_port) == NULL)
		{
			insert_into_sorted_servers(&destServer);
			g_tracker_server_count++;
		}
	}

	return 0;
}

int storage_load_from_conf_file(const char *conf_filename,\
			char *bind_addr,const int addr_size)
{

	char * pBasePath;
	char *pBindAddr;
	char *pGroupName;
	char *ppTrackerServers[FDFS_MAX_TRACKERS];
	IniItemInfo *items;
	int nItemCount;
	int result;

	if (0 != (result=iniLoadItems(conf_filename,&items,&nItemCount)))
	{
		logError("file:%s,line:%d," \
			"load from \"%s\" fail, ret code %d",
			__FILE__,__LINE__,conf_filename,result);

		return result;
	}

	while(1)
	{
		if (iniGetBoolValue("disabled", items, nItemCount))
		{
			logError("file:%s,line:%d," \
			"conf file \"%s\" disabled = true,exit",
			__FILE__,__LINE__,conf_filename);

			result = ECANCELED;
			break;
		}

		pBasePath = iniGetStrValue("base_path",items,nItemCount);
		if (NULL == pBasePath)
		{
			logError("file:%s,line:%d," \
			"conf file \"%s\" no has items,exit",
			__FILE__,__LINE__,conf_filename);

			result = ENOENT;
			break;
		}
		snprintf(g_base_path,sizeof(g_base_path),"%s",pBasePath);
		chopPath(g_base_path);
		if (!fileExists(g_base_path))
		{
			logError("file:%s,line:%d," \
			"can access file \"%s\" errno = %d,err info = %s",
			__FILE__,__LINE__,g_base_path,errno,strerror(errno));

			result = errno != 0 ? errno : ENOENT;
			break;
		}
		if (!isDir(g_base_path))
		{
			logError("file:%s,line:%d," \
			"\"%s\" is no dir\n",
			__FILE__,__LINE__,g_base_path,errno);

			result = ENOTDIR;
			break;
		}

		g_network_timeout = iniGetIntValue("network_tmeout",items,nItemCount,\
			DEFAULT_NETWORK_TIMEOUT);
		if (g_network_timeout <= 0)
		{
			g_network_timeout = DEFAULT_NETWORK_TIMEOUT;
		}

		g_server_port = iniGetIntValue("port",items,nItemCount,FDFS_STORAGE_SERVER_DEF_PORT);
		if (g_server_port <= 0)
		{
			g_server_port = FDFS_STORAGE_SERVER_DEF_PORT;
		}

		g_heart_beat_interval = iniGetIntValue("heart_beat_interval",items,nItemCount,\
			STORAGE_BEAT_DEF_INTERVAL);
		if (g_heart_beat_interval <= 0)
		{
			g_heart_beat_interval = STORAGE_BEAT_DEF_INTERVAL;
		}

		g_stat_report_interval = iniGetIntValue("stat_report_interval",items,nItemCount,\
			STORAGE_REPORT_DEF_INTERVAL);
		if (g_stat_report_interval <= 0)
		{
			g_stat_report_interval = STORAGE_REPORT_DEF_INTERVAL;
		}

		pBindAddr = iniGetStrValue("bind_addr",items,nItemCount);
		if (NULL == pBindAddr)
		{
			bind_addr[0] = '\0';
		}
		else
		{
			snprintf(bind_addr,sizeof(bind_addr),"%s",pBindAddr);
		}

		pGroupName = iniGetStrValue("group_name",items,nItemCount);
		if (NULL == pGroupName)
		{
			logError("file:%s,line:%d," \
			"conf file \"%s\" must have group name\n",
			__FILE__,__LINE__,conf_filename);

			result = ENOENT;
			break;
		}
		if ('\0' == pGroupName[0])
		{
			logError("file:%s,line:%d," \
			"conf file \"%s\"  group name if empty\n",
			__FILE__,__LINE__,conf_filename);

			result = EINVAL;
			break;
		}

		snprintf(g_group_name,sizeof(g_group_name),"%s",pGroupName);
		if (0 != (result=tracker_validate_group_name(g_group_name)) )
		{
			logError("file:%s,line:%d," \
			"conf file \"%s\"  group name \"%s\" if invalid\n",
			__FILE__,__LINE__,conf_filename,g_group_name);

			result = EINVAL;
			break;
		}

		g_tracker_server_count = iniGetValues("tracker_server", \
			items, nItemCount, ppTrackerServers,FDFS_MAX_TRACKERS);
		if (g_tracker_server_count <=0)
		{
			logError("file:%s,line:%d," \
			"conf file \"%s\"  get item tracker_server faild\n",
			__FILE__,__LINE__,conf_filename);

			result = ENOENT;
			break;
		}

		g_tracker_server = (TrackerServerInfo *)malloc( \
			sizeof(TrackerServerInfo) * g_tracker_server_count);
		if (NULL == g_tracker_server)
		{
			result = errno != 0 ? errno : ENOMEM;
			break;
		}

		memset(g_tracker_server,0,sizeof(TrackerServerInfo)*g_tracker_server_count);
		if ((result = copy_tracker_servers(conf_filename,ppTrackerServers)) != 0)
		{
			free(g_tracker_server);
			g_tracker_server = NULL;
			break;
		}

		g_sync_wait_usec = iniGetIntValue("sync_wait_usec",items,nItemCount,\
			STORAGE_DEF_SYNC_WAIT_MSEC);
		if (g_sync_wait_usec <=0)
		{
			g_sync_wait_usec = STORAGE_DEF_SYNC_WAIT_MSEC;
		}
		g_sync_wait_usec *= 1000;

		g_max_connections = iniGetIntValue("max_connections",items,nItemCount,
			FDFS_DEF_MAX_CONNECTONS);
		if (g_max_connections <=0)
		{
			g_max_connections = FDFS_DEF_MAX_CONNECTONS;
		}

		logInfo(STORAGE_ERROR_LOG_FILENAME, \
			"FastDFS v%d.%d, base_path=%s, " \
			"group_name=%s, " \
			"network_timeout=%d, "\
			"port=%d, bind_addr=%s, " \
			"max_connections=%d, "    \
			"heart_beat_interval=%ds, " \
			"stat_report_interval=%ds, tracker_server_count=%d, " \
			"sync_wait_usec=%dms", \
			g_version.major, g_version.minor, \
			g_base_path, g_group_name, \
			g_network_timeout, \
			g_server_port, bind_addr, g_max_connections, \
			g_heart_beat_interval, g_stat_report_interval, \
			g_tracker_server_count, g_sync_wait_usec / 1000);

		break;
	}

	iniFreeItems(items);

#ifdef __DEBUG__
	printf("storage_load_from_conf_file done!\n");
#endif
	return result;
}

static char * get_storage_stat_filename(const char *pArg,char *full_name)
{
	static char buff[MAX_PATH_SIZE];
	if (NULL == full_name)
	{
		full_name = buff;
	}
	snprintf(full_name, MAX_PATH_SIZE, \
			"%s/data/%s", g_base_path, \
			STORAGE_STAT_FILENAME);
	return full_name;
}

int storage_write_to_fd(int fd, get_filename_func filename_func, \
		const void *pArg, const char *buff, const int len)
{
	if (0 != ftruncate(fd,0))
	{
		logError("file:%s,line:%d," \
			"ftruncate \"%s\" to empty fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,filename_func(pArg, NULL),errno,strerror(errno));

		return errno !=0 ? errno:ENOENT;
	}

	if (0 != lseek(fd,0,SEEK_SET))
	{
		logError("file:%s,line:%d," \
			"lseek \"%s\" to start fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,filename_func(pArg, NULL),errno,strerror(errno));

		return errno !=0 ? errno:ENOENT;
	}

	if (len != write(fd,buff,len))
	{
		logError("file:%s,line:%d," \
			"write to \"%s\" fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,filename_func(pArg, NULL),errno,strerror(errno));

		return errno !=0 ? errno:ENOENT;
	}

	if (0 != fsync(fd))
	{
		logError("file:%s,line:%d," \
			"fsync file \"%s\" to disk fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,filename_func(pArg, NULL),errno,strerror(errno));

		return errno !=0 ? errno:ENOENT;
	}

	return 0;
}

int storage_write_to_stat_file()
{
	char buff[512];
	int len;

	len = sprintf(buff,
		"%s=%d\n"  \
		"%s=%d\n"  \
		"%s=%d\n"  \
		"%s=%d\n"  \
		"%s=%d\n"  \
		"%s=%d\n"  \
		"%s=%d\n"  \
		"%s=%d\n"  \
		"%s=%d\n"  \
		"%s=%d\n"  \
		"%s=%d\n"  \
		"%s=%d\n", \
		STAT_ITEM_TOTAL_UPLOAD, g_storage_stat.total_upload_count, \
		STAT_ITEM_SUCCESS_UPLOAD, g_storage_stat.success_upload_count, \
		STAT_ITEM_TOTAL_DOWNLOAD, g_storage_stat.total_download_count, \
		STAT_ITEM_SUCCESS_DOWNLOAD, \
		g_storage_stat.success_download_count, \
		STAT_ITEM_LAST_SOURCE_UPD, \
		(int)g_storage_stat.last_source_update, \
		STAT_ITEM_LAST_SYNC_UPD, (int)g_storage_stat.last_sync_update,\
		STAT_ITEM_TOTAL_SET_META, g_storage_stat.total_set_meta_count, \
		STAT_ITEM_SUCCESS_SET_META, \
		g_storage_stat.success_set_meta_count, \
		STAT_ITEM_TOTAL_DELETE, g_storage_stat.total_delete_count, \
		STAT_ITEM_SUCCESS_DELETE, g_storage_stat.success_delete_count, \
		STAT_ITEM_TOTAL_GET_META, g_storage_stat.total_get_meta_count, \
		STAT_ITEM_SUCCESS_GET_META, \
		g_storage_stat.success_get_meta_count
		);
	
	return storage_write_to_fd(storage_stat_fd,\
		get_storage_stat_filename,NULL,buff,len);

}

int storage_write_to_sync_ini_file()
{
	char full_name[MAX_PATH_SIZE];
	FILE *fp;

	snprintf(full_name,sizeof(full_name),"%s/data/%s", g_base_path, DATA_DIR_INITED_FILENAME);
	if (NULL ==(fp = fopen(full_name,"w")))
	{
		logError("file:%s,line:%d," \
			"fopen  \"%s\" fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,full_name ,errno,strerror(errno));

		return errno !=0 ? errno:ENOENT;
	}

	if (fprintf(fp, "%s=%d\n" \
		"%s=%d\n"  \
		"%s=%s\n"  \
		"%s=%d\n", \
		INIT_ITEM_STORAGE_JOIN_TIME, g_storage_join_time, \
		INIT_ITEM_SYNC_OLD_DONE, g_sync_old_done, \
		INIT_ITEM_SYNC_SRC_SERVER, g_sync_src_ip_addr, \
		INIT_ITEM_SYNC_UNTIL_TIMESTAMP, g_sync_until_timestamp \
		) <= 0)
	{
		logError("file:%s,line:%d," \
			"write to file \"%s\" fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,full_name ,errno,strerror(errno));

		fclose(fp);
		return errno !=0 ? errno:ENOENT;
	}
	fclose(fp);
	return 0;
}

int storage_check_and_make_data_dirs()
{

	char data_path[MAX_PATH_SIZE];
	char dir_name[MAX_PATH_SIZE];
	char sub_name[MAX_PATH_SIZE];

	int i,k;
	int result;

	snprintf(data_path,sizeof(data_path),"%s/data",g_base_path);
	if (!fileExists(data_path))
	{
		if (0 != mkdir(data_path,0755))
		{
			logError("file:%s,line:%d," \
				"mkdir \"%s\" fail ,errno = %d,err info = %s",
				__FILE__,__LINE__,data_path ,errno,strerror(errno));

			return errno !=0 ? errno:ENOENT;
		}
	}

	if (0 != chdir(data_path))
	{
		if (0 != mkdir(data_path,0755))
		{
			logError("file:%s,line:%d," \
				"chdir \"%s\" fail ,errno = %d,err info = %s",
				__FILE__,__LINE__,data_path ,errno,strerror(errno));

			return errno !=0 ? errno:ENOENT;
		}
	}

	if (fileExists(DATA_DIR_INITED_FILENAME))
	{




		;
	}

	g_storage_join_time =time(NULL);

	for (i = 0; i < DATA_DIR_COUNT_PER_PATH; ++i)
	{
		sprintf(dir_name,STORAGE_DATA_DIR_FORMAT,i);
		if ((0 != mkdir(dir_name,0755)) && (!fileExists(dir_name)))
		{
			if (!(EEXIST == errno && isDir(dir_name)))
			{
				logError("file:%s,line:%d," \
					"mkdir \"%s/%s\" fail ,errno = %d,err info = %s",
					__FILE__,__LINE__,data_path,dir_name ,errno,strerror(errno));

				return errno !=0 ? errno:ENOENT;
			}
		}

		if (0 != chdir(dir_name))
		{
			logError("file:%s,line:%d," \
				"chdir \"%s/%s\" fail ,errno = %d,err info = %s",
				__FILE__,__LINE__,data_path,dir_name ,errno,strerror(errno));

			return errno !=0 ? errno:ENOENT;
		}

		for (k = 0; k < DATA_DIR_COUNT_PER_PATH; ++k)
		{
			sprintf(sub_name,STORAGE_DATA_DIR_FORMAT,k);
			if ((0 != mkdir(sub_name,0755)) && (!fileExists(dir_name)))
			{
				if (!(EEXIST == errno && isDir(sub_name)))
				{
					logError("file:%s,line:%d," \
						"mkdir \"%s/%s/%s\" fail ,errno = %d,err info = %s",
						__FILE__,__LINE__,data_path,dir_name ,sub_name,errno,strerror(errno));

					return errno !=0 ? errno:ENOENT;
				}
			}
		}

		if (0 != chdir(".."))
		{
			logError("file:%s,line:%d," \
				"chdir \"%s\" fail ,errno = %d,err info = %s",
				__FILE__,__LINE__,data_path,errno,strerror(errno));

			return errno !=0 ? errno:ENOENT;
		}
	}

	result = storage_write_to_sync_ini_file();

#ifdef __DEBUG__
	printf("storage_check_and_make_data_dirs done!\n");
#endif
	return result;
}

int storage_open_storage_stat()
{

	char full_name[MAX_PATH_SIZE];
	IniItemInfo *items;
	int nItemCount;
	int result;

	get_storage_stat_filename(NULL,full_name);
	if (fileExists(full_name))
	{
		if (0 != (result =iniLoadItems(full_name, &items, &nItemCount)))
		{
			logError("file:%s,line:%d," \
				"iniLoadItems file \"%s\" fail ,errno = %d,err info = %s",
				__FILE__,__LINE__,full_name,errno,strerror(errno));

			return result;
		}
		if (nItemCount < 12)
		{
			logError("file:%s,line:%d," \
				"in file \"%s\" fail , nItemCount : %d<12",
				__FILE__,__LINE__,full_name,nItemCount);

			return ENOENT;
		}
		g_storage_stat.total_upload_count = iniGetIntValue( \
				STAT_ITEM_TOTAL_UPLOAD, \
				items, nItemCount, 0);
		g_storage_stat.success_upload_count = iniGetIntValue( \
				STAT_ITEM_SUCCESS_UPLOAD, \
				items, nItemCount, 0);
		g_storage_stat.total_download_count = iniGetIntValue( \
				STAT_ITEM_TOTAL_DOWNLOAD, \
				items, nItemCount, 0);
		g_storage_stat.success_download_count = iniGetIntValue( \
				STAT_ITEM_SUCCESS_DOWNLOAD, \
				items, nItemCount, 0);
		g_storage_stat.last_source_update = iniGetIntValue( \
				STAT_ITEM_LAST_SOURCE_UPD, \
				items, nItemCount, 0);
		g_storage_stat.last_sync_update = iniGetIntValue( \
				STAT_ITEM_LAST_SYNC_UPD, \
				items, nItemCount, 0);
		g_storage_stat.total_set_meta_count = iniGetIntValue( \
				STAT_ITEM_TOTAL_SET_META, \
				items, nItemCount, 0);
		g_storage_stat.success_set_meta_count = iniGetIntValue( \
				STAT_ITEM_SUCCESS_SET_META, \
				items, nItemCount, 0);
		g_storage_stat.total_delete_count = iniGetIntValue( \
				STAT_ITEM_TOTAL_DELETE, \
				items, nItemCount, 0);
		g_storage_stat.success_delete_count = iniGetIntValue( \
				STAT_ITEM_SUCCESS_DELETE, \
				items, nItemCount, 0);
		g_storage_stat.total_get_meta_count = iniGetIntValue( \
				STAT_ITEM_TOTAL_GET_META, \
				items, nItemCount, 0);
		g_storage_stat.success_get_meta_count = iniGetIntValue( \
				STAT_ITEM_SUCCESS_GET_META, \
				items, nItemCount, 0);

		iniFreeItems(items);


	}
	else
		memset(&g_storage_stat, 0, sizeof(g_storage_stat));

	storage_stat_fd = open(full_name,O_WRONLY | O_CREAT, 0644);
	if (storage_stat_fd <0)
	{
		logError("file:%s,line:%d," \
			"open file \"%s\" fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,full_name,errno,strerror(errno));

		return errno != 0 ? errno : ENOENT;
	}
	return storage_write_to_stat_file();

#ifdef __DEBUG__
	printf("storage_open_storage_stat done!\n");
#endif
	return 0;
}

int storage_close_storage_stat()
{
	printf("storage_close_storage_stat done!\n");
	return 0;
}