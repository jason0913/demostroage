#ifndef __FDFS_DEFINE_H_
#define __FDFS_DEFINE_H_

#ifndef true
typedef char  bool;
#define true  1
#define false 0
#endif

#define MAX_PATH_SIZE				256
#define LINE_MAX				256
#define FDFS_IPADDR_SIZE	16
#define STORAGE_ERROR_LOG_FILENAME      "storaged"
#define FDFS_STORAGE_SERVER_DEF_PORT		23000
#define FDFS_DEF_MAX_CONNECTONS			256

#endif