#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "storage_sync.h"
#include "fdfs_global.h"
#include "shared_func.h"
#include "logger.h"


#define SYNC_DIR_NAME			"sync"
#define SYNC_BINLOG_FILE_MAX_SIZE	1024 * 1024 * 1024
#define SYNC_BINLOG_FILE_PREFIX		"binlog"
#define SYNC_BINLOG_INDEX_FILENAME	SYNC_BINLOG_FILE_PREFIX".index"
#define SYNC_BINLOG_FILE_EXT_FMT	".%03d"

int g_binlog_index = 0;
FILE *g_fp_binlog = NULL;
static int binlog_file_size = 0;
static pthread_mutex_t sync_thread_lock;

int g_storage_sync_thread_count = 0;


static int write_to_binlog_index()
{
	char full_filename[MAX_PATH_SIZE];
	FILE * fp;

	snprintf(full_filename,sizeof(full_filename),"%s/data/"SYNC_DIR_NAME"/%s",\
		g_base_path,SYNC_BINLOG_INDEX_FILENAME);
	if ((fp=fopen(full_filename, "wb")) == NULL)
	{
		/*printf("file:%s,line:%d,\
			open:\"%s\" failed,errno= %d,err info = %s",
			__FILE__,__LINE__,full_filename,errno,strerror(errno));*/

		logError("file:%s,line:%d,"\
			"open:\"%s\" failed,errno= %d,err info = %s",
			__FILE__,__LINE__,full_filename,errno,strerror(errno));

		return errno != 0? errno:ENOENT;
	}

	if (fprintf(fp, "%d", g_binlog_index) <0)
	{
		logError("file:%s,line:%d,"\
			"write to file:\"%s\" failed,errno= %d,err info = %s",
			__FILE__,__LINE__,full_filename,errno,strerror(errno));

		fclose(fp);
		return errno != 0? errno:ENOENT;
	}

	fclose(fp);
	return 0;
}

int storage_sync_init()
{

	char data_path[MAX_PATH_SIZE];
	char sync_path[MAX_PATH_SIZE];
	char full_filename[MAX_PATH_SIZE];
	char file_buff[MAX_PATH_SIZE];

	int bytes;
	int result;
	FILE *fp;

	snprintf(data_path,sizeof(data_path),"%s/data",g_base_path);
	if (!fileExists(data_path))
	{
		if (0 != mkdir(data_path,0755))
		{
			logError("file:%s,line:%d,"\
				"mkdir:\"%s\" failed,errno= %d,err info = %s",
				__FILE__,__LINE__,data_path,errno,strerror(errno));

			return errno != 0? errno:ENOENT;
		}

	}

	snprintf(sync_path,sizeof(sync_path),"%s/"SYNC_DIR_NAME,g_base_path);
	if (!fileExists(sync_path))
	{
		if (0 != mkdir(sync_path,0755))
		{
			logError("file:%s,line:%d,"\
				"mkdir:\"%s\" failed,errno= %d,err info = %s",
				__FILE__,__LINE__,sync_path,errno,strerror(errno));

			return errno != 0? errno:ENOENT;
		}

	}

	snprintf(full_filename,sizeof(full_filename),"%s/%s", sync_path, SYNC_BINLOG_INDEX_FILENAME);
	if (NULL !=(fp = fopen(full_filename,"rb")))
	{
		if ((bytes = fread(file_buff,1,sizeof(file_buff)-1,fp)) <=0)
		{
			logError("file:%s,line:%d,"\
				"read file \"%s\" failed,bytes read: %d",
				__FILE__,__LINE__,full_filename,bytes);

			return errno != 0? errno:ENOENT;
		}
		file_buff[bytes] = '\0';
		g_binlog_index = atoi(file_buff);
		fclose(fp);
		if (g_binlog_index < 0)
		{
			logError("file:%s,line:%d," \
				"in file \"%s\", g_binlog_index= %d <0",
				__FILE__,__LINE__,full_filename,g_binlog_index);

			return EINVAL;
		}
	}
	else
	{
		g_binlog_index =0;
		if ((result=write_to_binlog_index()) != 0)
		{
			return 0;
			// return result;
		}
	}

	//get_writable_binlog_filename(full_filename);
	g_fp_binlog = fopen(full_filename,"a");
	if (NULL == g_fp_binlog)
	{
		logError("file:%s,line:%d," \
				"open file \"%s\", errno= %d,err info = %s",
				__FILE__,__LINE__,full_filename,errno,strerror(errno));

		return errno != 0? errno :ENOENT;
	}

	binlog_file_size = ftell(g_fp_binlog);
	if (binlog_file_size <0)
	{
		logError("file:%s,line:%d," \
				"ftell file \"%s\", errno= %d,err info = %s",
				__FILE__,__LINE__,full_filename,errno,strerror(errno));

		storage_sync_destroy();
		return errno != 0 ? errno : ENOENT;
	}

	if (0 != (result =init_pthread_lock(&sync_thread_lock)))
	{
		return result;
	}

	//load_local_host_ip_addrs();
#ifdef __DEBUG__
	printf("storage_sync_init done!\n");
#endif

	return 0;
}

int storage_sync_destroy()
{

	if (NULL != g_fp_binlog)
	{
		g_fp_binlog = NULL;
		fclose(g_fp_binlog);
	}
	if (0 != pthread_mutex_destroy(&sync_thread_lock))
	{
		logError("file:%s,line:%d," \
				"pthread_mutex_destroy sync_thread_lock fail, errno= %d,err info = %s",
				__FILE__,__LINE__,errno,strerror(errno));

		return errno !=0 ? errno: ENOENT;

	}
#ifdef __DEBUG__
	printf("storage_sync_destroy done!\n");
#endif

	return 0;
}

static char *get_writable_binlog_filename(char *full_filename)
{
	static char buff[MAX_PATH_SIZE];

	if (NULL == full_filename)
	{
		full_filename = buff;
	}

	snprintf(full_filename, MAX_PATH_SIZE, \
			"%s/data/"SYNC_DIR_NAME"/"SYNC_BINLOG_FILE_PREFIX"" \
			SYNC_BINLOG_FILE_EXT_FMT, \
			g_base_path, g_binlog_index);

	return full_filename;
}

static int open_next_writable_binlog()
{
	char full_filename[MAX_PATH_SIZE];

	storage_sync_destroy();

	get_writable_binlog_filename(full_filename);
	if (fileExists(full_filename))
	{
		if (0 != unlink(full_filename))
		{
			logError("file: %s, line: %d, " \
				"unlink file \"%s\" fail, " \
				"errno: %d, error info: %s", \
				__FILE__,__LINE__, full_filename, \
				errno, strerror(errno));
			return errno != 0 ? errno : ENOENT;
		}

		logError("file: %s, line: %d, " \
			"binlog file \"%s\" already exists, truncate", \
			__FILE__,__LINE__, full_filename);
	}

	g_fp_binlog = fopen(full_filename,"r");
	if (NULL == g_fp_binlog)
	{
		logError("file: %s, line: %d, " \
			"open file \"%s\" fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, full_filename, \
			errno, strerror(errno));
		return errno != 0 ? errno : ENOENT;
	}

	return 0;
}

int storage_binlog_write(const char op_type, const char *filename)
{
	int fd;
	struct flock lock;
	int write_bytes;
	int result;

	fd = fileno(g_fp_binlog);

	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_len = 10;
	lock.l_whence = SEEK_SET;

	if (0 != fcntl(fd,F_SETLKW,&lock))
	{
		logError("file: %s, line: %d, " \
			"lock binlog file \"%s\" fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, get_writable_binlog_filename(NULL), \
			errno, strerror(errno));

		return errno != 0 ? errno : ENOENT;
	}

	write_bytes = fprintf(g_fp_binlog, "%d %c %s\n", \
			(int)time(NULL), op_type, filename);

	if (write_bytes <= 0)
	{
		logError("file: %s, line: %d, " \
			"write to binlog file \"%s\" fail, " \
			"errno: %d, error info: %s",  \
			__LINE__, get_writable_binlog_filename(NULL), \
			errno, strerror(errno));

		result = errno != 0 ? errno : ENOENT;
	}
	else if (0 != fflush(g_fp_binlog))
	{
		logError("file: %s, line: %d, " \
			"sync to binlog file \"%s\" fail, " \
			"errno: %d, error info: %s",  \
			__FILE__,__LINE__, get_writable_binlog_filename(NULL), \
			errno, strerror(errno));

		result = errno != 0 ? errno : ENOENT;
	}
	else
	{
		binlog_file_size += write_bytes;
		if (binlog_file_size >= SYNC_BINLOG_FILE_MAX_SIZE)
		{
			g_binlog_index++;
			if (0 == (result = write_to_binlog_index()))
			{
				result = open_next_writable_binlog();
			}

			binlog_file_size = 0;
			if (0 != result)
			{
				g_continue_flag = false;
				logError("file: %s, line: %d, " \
					"open binlog file \"%s\" fail, " \
					"process exit!", \
					__FILE__,__LINE__, \
					get_writable_binlog_filename(NULL));
			}
		}
		else
		{
			result = 0;
		}
	}

	lock.l_type = F_UNLCK;
	if (fcntl(fd, F_SETLKW, &lock) != 0)
	{
		logError("file: %s, line: %d, " \
			"unlock binlog file \"%s\" fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, get_writable_binlog_filename(NULL), \
			errno, strerror(errno));
		return errno != 0 ? errno : ENOENT;
	}

	return result;
}