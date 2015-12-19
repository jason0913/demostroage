#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "tracker_client_thread.h"
#include "shared_func.h"
#include "tracker_types.h"
#include "storage_global.h"
#include "logger.h"

static pthread_mutex_t reporter_thread_lock;

int tracker_report_init()
{

	int result;
	if (0 !=(result =init_pthread_lock(&reporter_thread_lock)))
	{
		return result;
	}

#ifdef __DEBUG__
	printf("tracker_report_init done!\n");
#endif

	return 0;
}

int tracker_report_destroy()
{
	printf("tracker_report_destroy done!\n");
	return 0;
}

static void* tracker_report_thread_entrance(void* arg)
{
	printf("tracker_report_thread_entrance! finish me. ###########>>>\n");

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
	printf("tracker_report_thread_start done!\n");
#endif
	return 0;
}