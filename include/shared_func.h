#ifndef __SHARED_FUNC
#define __SHARED_FUNC

#include <pthread.h>
#include "fdfs_define.h"

extern void daemon_init(bool bCloseFiles);
extern int init_pthread_lock(pthread_mutex_t *pthread_lock);

extern bool fileExists(const char *filename);
extern bool isDir(const char *filename);
extern void chopPath(char *filePath);

extern char *trim_left(char *pStr);
extern char *trim_right(char *pStr);
extern char *trim(char *pStr);

extern void int2buff(const int n, char *buff);

#endif