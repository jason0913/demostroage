#ifndef __STORAGE_SYNC_H_
#define __STORAGE_SYNC_H_

#define STORAGE_OP_TYPE_SOURCE_CREATE_FILE	'C'
#define STORAGE_OP_TYPE_SOURCE_DELETE_FILE	'D'
#define STORAGE_OP_TYPE_SOURCE_UPDATE_FILE	'U'
#define STORAGE_OP_TYPE_REPLICA_CREATE_FILE	'c'
#define STORAGE_OP_TYPE_REPLICA_DELETE_FILE	'd'
#define STORAGE_OP_TYPE_REPLICA_UPDATE_FILE	'u'

extern int g_binlog_index;
extern FILE *g_fp_binlog;

extern int g_storage_sync_thread_count;

extern int storage_sync_init();
extern int storage_sync_destroy();

extern int storage_binlog_write(const char op_type, const char *filename);

#endif