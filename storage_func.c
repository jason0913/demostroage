#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "storage_func.h"
#include "logger.h"
#include "fdfs_global.h"
#include "shared_func.h"
#include "ini_file_reader.h"

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

int storage_load_from_conf_file(const char *conf_filename,\
			char *bind_addr,const int addr_size)
{
	printf("storage_load_from_conf_file done!\n");
	return 0;
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