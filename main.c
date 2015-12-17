#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "logger.h"
#include "storage_func.h"
#include "storage_sync.h"
#include "sockopt.h"
#include "tracker_client_thread.h"
#include "shared_func.h"
#include "fdfs_global.h"

bool bReloadFlag = false;

void sigIntHandler(int sig);
void sigQuitHandler(int sig);
void sigHupHandler(int sig);

int setRandSeed();

int main(int argc, char const *argv[])
{
	char * conf_filename;
	char bind_addr[FDFS_IPADDR_SIZE];
	pthread_attr_t pth_attr;
	int comesock;

	pthread_t tid;
	int result;
	int sock;

	if (argc < 2)
	{
		printf("Usage: demostorage conf_filename\n");
		return 1;
	}

	conf_filename = argv[1];
	memset(bind_addr,0,sizeof(bind_addr));

	if (0 !=(result = storage_load_from_conf_file(conf_filename,bind_addr,sizeof(bind_addr))))
	{
		printf("storage_load_from_conf_file failed!\n");
		return result;
	}

	if (0 != (result =check_and_mk_log_dir()))
	{
		printf("check_and_mk_log_dir failed!\n");
		return result;
	}

	sock = socketServer(bind_addr,g_server_port,STORAGE_ERROR_LOG_FILENAME);
	if (sock <0)
	{
		printf("socketServer failed!\n");
		return 5;
	}

	if (0 != (result = storage_sync_init()))
	{
		printf("storage_sync_init failed!\n");
		g_continue_flag = false;
		return result;
	}

	if (0 != (result = tracker_report_init()))
	{
		printf("tracker_report_init failed!\n");
		g_continue_flag = false;
		return result;
	}

	if (0 != (result = storage_check_and_make_data_dirs()))
	{
		printf("storage_check_and_make_data_dirs failed!\n");
		g_continue_flag = false;
		return result;
	}

	if (0 !=(result = init_pthread_lock(&g_storage_thread_lock)))
	{
		printf("init_pthread_lock failed!\n");
		g_continue_flag = false;
		return result;
	}

	if (0 != (result = setRandSeed()))
	{
		printf("setRandSeed failed!\n");
		g_continue_flag = false;
		return result;
	}

	if (0 != (result = storage_open_storage_stat()))
	{
		printf("storage_open_storage_stat failed!\n");
		g_continue_flag = false;
		return result;
	}

	daemon_init(false);
	umask(0);

	g_storage_thread_count =0;
	pthread_attr_init(&pth_attr);
	result = pthread_attr_setdetachstate(&pth_attr, PTHREAD_CREATE_DETACHED);

	if (0 !=(result = tracker_report_thread_start()))
	{
		printf("tracker_report_thread_start failed!\n");
		g_continue_flag = false;
		storage_close_storage_stat();
		return result;
	}

	signal(SIGINT,sigIntHandler);
	signal(SIGQUIT,sigQuitHandler);
	signal(SIGTERM,sigQuitHandler);
	signal(SIGHUP,sigHupHandler);
	signal(SIGPIPE,SIG_IGN);

	while(g_continue_flag)
	{
		comesock = nbaccept(sock,60,&result);
		if (comesock <0)
		{
			printf("nbaccept failed\n");
			continue;
		}

		if (0 != pthread_mutex_lock(&g_storage_thread_lock))
		{
			printf("pthread_mutex_lock faile\n");
		}

		if (g_storage_thread_count >  g_max_connections)
		{
			printf("g_storage_thread_count exceed g_max_connections\n");
			close(comesock);
		}
		else
		{

		}

		if (0 != pthread_mutex_unlock(&g_storage_thread_lock))
		{
			printf("pthread_mutex_unlock\n");
		}
		printf("g_continue_flag main sleep...\n");
		sleep(1);
	}
	while(0 != g_storage_thread_count)
	{
		sleep(1);
	}

	pthread_attr_destroy(&pth_attr);
	pthread_mutex_destroy(&g_storage_thread_lock);

	storage_sync_destroy();
	storage_close_storage_stat();

	printf("exit normally\n");
	
	return 0;
}

void sigIntHandler(int sig)
{
	g_continue_flag = false;
}
void sigQuitHandler(int sig)
{
	g_continue_flag = false;
}
void sigHupHandler(int sig)
{
	bReloadFlag = true;
}

int setRandSeed()
{

	struct timeval tv;

	if (0 != gettimeofday(&tv,NULL))
	{
		logError("file: %s, line: %d, " \
					 "call gettimeofday fail, " \
					 "errno=%d, error info: %s", \
					 __FILE__,__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EPERM;
	}

	srand(tv.tv_sec ^ tv.tv_usec);

#ifdef __DEBUG__
	printf("setRandSeed done\n");
#endif

	return 0;
}
