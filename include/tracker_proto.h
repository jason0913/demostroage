#ifndef __TRACKER_PROTO_H
#define __TRACKER_PROTO_H

#include "tracker_types.h"

#define TRACKER_PROTO_CMD_STORAGE_JOIN          81
#define TRACKER_PROTO_CMD_STORAGE_QUIT          82
#define TRACKER_PROTO_CMD_STORAGE_BEAT          83  //heart beat
#define TRACKER_PROTO_CMD_STORAGE_REPORT        84
#define TRACKER_PROTO_CMD_STORAGE_REPLICA_CHG   85  //repl new storage servers
#define TRACKER_PROTO_CMD_STORAGE_SYNC_SRC_REQ  86  //src storage require sync
#define TRACKER_PROTO_CMD_STORAGE_SYNC_DEST_REQ 87  //dest storage require sync
#define TRACKER_PROTO_CMD_STORAGE_SYNC_NOTIFY   88  //sync notify
#define TRACKER_PROTO_CMD_STORAGE_RESP          80

#define TRACKER_PROTO_PKG_LEN_SIZE	9
#define TRACKER_PROTO_CMD_SIZE		1

typedef struct
{
	char pkg_len[TRACKER_PROTO_PKG_LEN_SIZE];
	char cmd;
	char status;
} TrackerHeader;

typedef struct
{
	char group_name[FDFS_GROUP_NAME_MAX_LEN+1];
	char storage_port[TRACKER_PROTO_PKG_LEN_SIZE];
} TrackerStorageJoinBody;

typedef struct
{
	char sz_total_mb[4];
	char sz_free_mb[4];
} TrackerStatReportReqBody;

typedef struct
{
	char src_ip_addr[FDFS_IPADDR_SIZE];
	char until_timestamp[TRACKER_PROTO_PKG_LEN_SIZE];
} TrackerStorageSyncReqBody;

extern int tracker_validate_group_name(const char *group_name);

extern int tracker_quit(TrackerServerInfo *pTrackerServer);

extern int tracker_recv_response(TrackerServerInfo *pTrackerServer, \
		char **buff, const int buff_size, \
		int *in_bytes);
#endif