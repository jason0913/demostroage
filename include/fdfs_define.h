#ifndef __FDFS_DEFINE_H_
#define __FDFS_DEFINE_H_

#define FDFS_RECORD_SEPERATOR	'\x01'
#define FDFS_FIELD_SEPERATOR	'\x02'

#ifndef true
typedef char  bool;
#define true  1
#define false 0
#endif

#define MAX_PATH_SIZE				256
#define LINE_MAX				256
#define FDFS_IPADDR_SIZE	16
#define STORAGE_ERROR_LOG_FILENAME      "storaged"
#define DEFAULT_NETWORK_TIMEOUT			30
#define FDFS_STORAGE_SERVER_DEF_PORT		23000
#define FDFS_TRACKER_SERVER_DEF_PORT		22000
#define FDFS_DEF_MAX_CONNECTONS			256

#define STORAGE_BEAT_DEF_INTERVAL    30
#define STORAGE_REPORT_DEF_INTERVAL  300
#define STORAGE_DEF_SYNC_WAIT_MSEC   100
#define STORAGE_SYNC_STAT_FILE_FREQ  1000

#endif