#include <stdio.h>
#include "tracker_client_thread.h"
#include "shared_func.h"

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
int tracker_report_thread_start()
{
	printf("tracker_report_thread_start done!\n");
	return 0;
}