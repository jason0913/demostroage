#include <stdio.h>
#include <pthread.h>
#include "storage_func.h"
#include "fdfs_define.h"

void daemon_init(bool bCloseFiles)
{
	printf("daemon_init done\n");
	return;
}
int init_pthread_lock(pthread_mutex_t *pthread_lock)
{
	printf("init_pthread_lock done\n");
	return 0;
}