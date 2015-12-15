#ifndef __FDFS_GLOBAL_H_
#define __FDFS_GLOBAL_H_

#include "fdfs_define.h"

int g_server_port = FDFS_STORAGE_SERVER_DEF_PORT;
int g_max_connections = FDFS_DEF_MAX_CONNECTONS;
int g_storage_thread_count = 0;
bool g_continue_flag = true;

pthread_mutex_t g_storage_thread_lock;
#endif