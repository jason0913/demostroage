#ifndef __TRACKER_TYPES_H_
#define __TRACKER_TYPES_H_

#include <time.h>
#include "fdfs_define.h"

#define FDFS_ONE_MB	(1024 * 1024)

#define FDFS_MAX_TRACKERS		16
#define FDFS_GROUP_NAME_MAX_LEN		16

#define FDFS_MAX_META_NAME_LEN		64
#define FDFS_MAX_META_VALUE_LEN		256

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

typedef struct
{
	char sz_total_upload_count[4];
	char sz_success_upload_count[4];
	char sz_total_set_meta_count[4];
	char sz_success_set_meta_count[4];
	char sz_total_delete_count[4];
	char sz_success_delete_count[4];
	char sz_total_download_count[4];
	char sz_success_download_count[4];
	char sz_total_get_meta_count[4];
	char sz_success_get_meta_count[4];
	char sz_last_source_update[4];
	char sz_last_sync_update[4];
} FDFSStorageStatBuff;

typedef struct
{
	int sock;
	char ip_addr[FDFS_IPADDR_SIZE];
} StorageClientInfo;

typedef struct
{
	char name[FDFS_MAX_META_NAME_LEN + 1];
	char value[FDFS_MAX_META_VALUE_LEN + 1];
} FDFSMetaData;

#endif