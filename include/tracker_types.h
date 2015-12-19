#ifndef __TRACKER_TYPES_H_
#define __TRACKER_TYPES_H_

#include <time.h>
#include "fdfs_define.h"

#define FDFS_MAX_TRACKERS		16
#define FDFS_GROUP_NAME_MAX_LEN		16

typedef struct
{
	int total_upload_count;
	int success_upload_count;
	int total_set_meta_count;
	int success_set_meta_count;
	int total_delete_count;
	int success_delete_count;
	int total_download_count;
	int success_download_count;
	int total_get_meta_count;
	int success_get_meta_count;
	time_t last_source_update;
	time_t last_sync_update;
	/*
	int total_check_count;
	int success_check_count;
	time_t last_check_time;
	*/
} FDFSStorageStat;

typedef struct
{
	int sock;
	int port;
	char ip_addr[FDFS_IPADDR_SIZE];
	char group_name[FDFS_GROUP_NAME_MAX_LEN + 1];
} TrackerServerInfo;

#endif