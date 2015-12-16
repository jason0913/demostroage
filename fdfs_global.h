#ifndef __FDFS_GLOBAL_H_
#define __FDFS_GLOBAL_H_

#include "fdfs_define.h"
#include <pthread.h>

extern int g_server_port ;
extern int g_max_connections;
extern int g_storage_thread_count;
extern bool g_continue_flag;
extern char g_base_path[MAX_PATH_SIZE];

extern pthread_mutex_t g_storage_thread_lock;

#endif