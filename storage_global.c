#include <string.h>
#include "fdfs_define.h"
#include "storage_global.h"

int g_tracker_server_count = 0;
TrackerServerInfo *g_tracker_server = NULL;

int g_local_host_ip_count = 0;
char g_local_host_ip_addrs[STORAGE_MAX_LOCAL_IP_ADDRS * \
				FDFS_IPADDR_SIZE];
int g_stat_change_count = 1;

bool is_local_host_ip(const char *client_ip)
{
	char *p;
	char *pEnd;

	pEnd = g_local_host_ip_addrs + FDFS_IPADDR_SIZE * g_local_host_ip_count;
	for (p = g_local_host_ip_addrs; p < pEnd; p++)
	{
		if (0 == strcmp(client_ip,p))
		{
			return true;
		}
	}
	return false;
}

int insert_into_local_host_ip(const char *client_ip)
{
	if (is_local_host_ip(client_ip))
	{
		return 0;
	}

	if (g_local_host_ip_count >= STORAGE_MAX_LOCAL_IP_ADDRS)
	{
		return -1;
	}

	strcpy(g_local_host_ip_addrs + \
		FDFS_IPADDR_SIZE * g_local_host_ip_count, \
		client_ip);
	g_local_host_ip_count++;

	return 1;
}