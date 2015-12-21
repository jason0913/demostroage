#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "tracker_proto.h"
#include "sockopt.h"
#include "logger.h"
#include "fdfs_global.h"

int tracker_validate_group_name(const char *group_name)
{
	const char *p;
	const char *pEnd;
	int len;

	len = strlen(group_name);
	if (0 == len)
	{
		return EINVAL;
	}

	pEnd = group_name + len;
	for (p = group_name; p < pEnd; p++)
	{
		if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || \
			(*p >= '0' && *p <= '9')))
		{
			return EINVAL;
		}
	}
	return 0;
}

int tracker_quit(TrackerServerInfo *pTrackerServer)
{

	TrackerHeader header;
	int result;

	header.pkg_len[0] = '0';
	header.pkg_len[1] = '\0';
	header.cmd = TRACKER_PROTO_CMD_STORAGE_QUIT;
	header.status = 0;

	result = tcpsenddata(pTrackerServer->sock, &header, sizeof(header), \
				g_network_timeout);
	if (result != 1)
	{
		logError("file: %s, line: %d, " \
			"tracker server ip: %s, send data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			errno, strerror(errno));
		return errno != 0 ? errno : EPIPE;
	}

	return 0;
}

int tracker_recv_response(TrackerServerInfo *pTrackerServer, \
		char **buff, const int buff_size, \
		int *in_bytes)
{

	TrackerHeader resp;
	bool bMalloced;

	if (1 != tcprecvdata(pTrackerServer->sock,&resp,sizeof(resp),\
		g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"server: %s:%d, recv data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, \
			errno, strerror(errno));
		*in_bytes = 0;
		return errno != 0 ? errno : EPIPE;
	}

	if (0 != resp.status)
	{
		*in_bytes = 0;
		return resp.status;
	}

	resp.pkg_len[TRACKER_PROTO_PKG_LEN_SIZE-1] = '\0';
	*in_bytes = strtol(resp.pkg_len,NULL,16);
	if (*in_bytes < 0)
	{
		logError("file: %s, line: %d, " \
			"server: %s:%d, recv package size %d " \
			"is not correct", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, *in_bytes);

		*in_bytes = 0;
		return errno != 0 ? errno : EINVAL;
	}

	if (0 == *in_bytes)
	{
		return resp.status;
	}

	if (NULL == *buff)
	{
		*buff = (char *)malloc((*in_bytes) + 1);
		if (NULL == *buff)
		{
			*in_bytes = 0;
			return errno != 0 ? errno : ENOMEM;
		}

		bMalloced = true;
	}
	else
	{
		if (*in_bytes > buff_size)
		{
			logError("file: %s, line: %d, " \
				"server: %s:%d, recv body bytes: %d" \
				" exceed max: %d", \
				__FILE__,__LINE__, pTrackerServer->ip_addr, \
				pTrackerServer->port, *in_bytes, buff_size);
			*in_bytes = 0;
			return errno != 0 ? errno : EINVAL;
		}

		bMalloced = false;
	}

	if (1 != tcprecvdata(pTrackerServer->sock,*buff,*in_bytes,\
		g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"tracker server: %s:%d, recv data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, \
			errno, strerror(errno));

		*in_bytes = 0;
		if (bMalloced)
		{
			free(*buff);
			*buff = NULL;
		}
		return errno != 0 ? errno : EPIPE;
	}

	return resp.status;
}