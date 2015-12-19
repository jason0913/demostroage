#ifndef __STORAGE_SYNC_H_
#define __STORAGE_SYNC_H_

extern int g_binlog_index;
extern FILE *g_fp_binlog;

extern int g_storage_sync_thread_count;

extern int storage_sync_init();
extern int storage_sync_destroy();

#endif