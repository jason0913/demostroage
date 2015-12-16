#ifndef __SHARED_FUNC
#define __SHARED_FUNC

#include <pthread.h>
#include "fdfs_define.h"

extern void daemon_init(bool bCloseFiles);
extern int init_pthread_lock(pthread_mutex_t *pthread_lock);

#endif