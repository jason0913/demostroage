#ifndef __FDFS_GLOBAL_H_
#define __FDFS_GLOBAL_H_

#include "fdfs_define.h"
#include <pthread.h>

extern int g_server_port ;
extern int g_max_connections;
extern int g_storage_thread_count;
extern bool g_continue_flag;
extern char g_base_path[MAX_PATH_SIZE];

extern int g_storage_join_time;
extern int g_storage_join_time;
extern int g_storage_join_time;
extern bool g_sync_old_done;
extern char g_sync_src_ip_addr[FDFS_IPADDR_SIZE];
extern int g_sync_until_timestamp;

extern pthread_mutex_t g_storage_thread_lock;

#endif