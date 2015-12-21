#ifndef __CLIENT_GLOBAL_H
#define __CLIENT_GLOBAL_H

#include "tracker_types.h"

#define STORAGE_MAX_LOCAL_IP_ADDRS	4

extern int g_tracker_server_count;
extern int g_tracker_server_index;  //server index for roundrobin
extern TrackerServerInfo *g_tracker_server;

extern int g_local_host_ip_count;
extern char g_local_host_ip_addrs[STORAGE_MAX_LOCAL_IP_ADDRS * \
				FDFS_IPADDR_SIZE];
extern int g_stat_change_count;

extern bool is_local_host_ip(const char *client_ip);
extern int insert_into_local_host_ip(const char *client_ip);

#endif