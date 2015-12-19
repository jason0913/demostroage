#include "fdfs_global.h"

int g_server_port = FDFS_STORAGE_SERVER_DEF_PORT;
int g_max_connections = FDFS_DEF_MAX_CONNECTONS;
int g_storage_thread_count = 0;
bool g_continue_flag = true;
char g_base_path[MAX_PATH_SIZE];
int g_network_timeout = DEFAULT_NETWORK_TIMEOUT;

FDFSVersion g_version = {1, 2};
FDFSStorageStat g_storage_stat;

char g_group_name[FDFS_GROUP_NAME_MAX_LEN + 1];
int g_tracker_reporter_count = 0;
int g_heart_beat_interval  = STORAGE_BEAT_DEF_INTERVAL;
int g_stat_report_interval = STORAGE_REPORT_DEF_INTERVAL;
int g_sync_wait_usec = STORAGE_DEF_SYNC_WAIT_MSEC;

int g_storage_join_time = 0;
bool g_sync_old_done = false;
char g_sync_src_ip_addr[FDFS_IPADDR_SIZE] = {0};
int g_sync_until_timestamp = 0;

pthread_mutex_t g_storage_thread_lock;

