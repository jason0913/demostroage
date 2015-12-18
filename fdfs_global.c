#include "fdfs_global.h"

int g_server_port = FDFS_STORAGE_SERVER_DEF_PORT;
int g_max_connections = FDFS_DEF_MAX_CONNECTONS;
int g_storage_thread_count = 0;
bool g_continue_flag = true;
char g_base_path[MAX_PATH_SIZE];

FDFSStorageStat g_storage_stat;

int g_storage_join_time = 0;
bool g_sync_old_done = false;
char g_sync_src_ip_addr[FDFS_IPADDR_SIZE] = {0};
int g_sync_until_timestamp = 0;

pthread_mutex_t g_storage_thread_lock;

