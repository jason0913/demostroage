#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "storage_service.h"
#include "sockopt.h"
#include "logger.h"
#include "tracker_types.h"
#include "tracker_proto.h"
#include "fdfs_global.h"
#include "storage_func.h"
#include "storage_global.h"
#include "shared_func.h"
#include "storage_sync.h"
#include "fdfs_base64.h"
#include "hash.h"

#define CHECK_AND_WRITE_TO_STAT_FILE  \
		if (++g_stat_change_count % STORAGE_SYNC_STAT_FILE_FREQ == 0) \
		{ \
			if (storage_write_to_stat_file() != 0) \
			{ \
				break; \
			} \
		}

static int storage_gen_filename(StorageClientInfo *pClientInfo, \
			const int file_size, \
			char *filename, int *filename_len)
{
	//struct timeval tv;
	int current_time;
	int r;
	char buff[sizeof(int) * 3];
	char encoded[sizeof(int) * 4 + 1];
	int n;
	int len;

	/*
	if (gettimeofday(&tv, NULL) != 0)
	{
		logError("client ip: %s, call gettimeofday fail, " \
			 "errno=%d, error info: %s", \
			__LINE__, pClientInfo->ip_addr,  \
			 errno, strerror(errno));
		return errno != 0 ? errno : EPERM;
	}

	current_time = (int)(tv.tv_sec - (2008 - 1970) * 365 * 24 * 3600) * \
			100 + (int)(tv.tv_usec / 10000);
	*/

	r = rand();
	current_time = time(NULL);
	int2buff(current_time, buff);
	int2buff(file_size, buff+sizeof(int));
	int2buff(r, buff+sizeof(int)*2);

	base64_encode_ex(buff, sizeof(int) * 3, encoded, filename_len, false);
	n = PJWHash(encoded, *filename_len) % (1 << 16);
	len = sprintf(buff, STORAGE_DATA_DIR_FORMAT"/", (n >> 8) & 0xFF);
	len += sprintf(buff + len, STORAGE_DATA_DIR_FORMAT"/", n & 0xFF);

	memcpy(filename, buff, len);
	memcpy(filename+len, encoded, *filename_len+1);
        *filename_len += len;

	return 0;
}

static int storage_do_set_metadata(StorageClientInfo *pClientInfo, \
		const char *full_meta_filename, char *meta_buff, \
		const int meta_bytes, const char op_flag, char *sync_flag)
{

#ifdef __DEBUG__
	printf("to be finish ---->>>>> file:%s,line:%s\n", __FILE__,__LINE__);
#endif
	return 0;
}

static int storage_download_file(StorageClientInfo *pClientInfo, \
				const int nInPackLen)
{
	TrackerHeader resp;
	int result;
	char in_buff[FDFS_GROUP_NAME_MAX_LEN + 32];
	char group_name[FDFS_GROUP_NAME_MAX_LEN + 1];
	char full_filename[MAX_PATH_SIZE+sizeof(in_buff)+16];
	char *file_buff;
	int file_bytes;

	file_buff = NULL;
	file_bytes = 0;

	while(1)
	{
		if (nInPackLen <= FDFS_GROUP_NAME_MAX_LEN)
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is not correct, " \
				"expect length > %d", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_UPLOAD_FILE, \
				pClientInfo->ip_addr,  \
				nInPackLen, FDFS_GROUP_NAME_MAX_LEN);
			resp.status = EINVAL;
			break;
		}

		if (nInPackLen >= sizeof(in_buff))
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is too large, " \
				"expect length should < %d", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_UPLOAD_FILE, \
				pClientInfo->ip_addr,  \
				nInPackLen, sizeof(in_buff));
			resp.status = EINVAL;
			break;
		}

		if (1 != tcprecvdata(pClientInfo->sock,in_buff,nInPackLen,g_network_timeout))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, recv data fail, " \
				"errno: %d, error info: %s.", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				errno, strerror(errno));
			resp.status = errno != 0 ? errno : EPIPE;
			break;
		}

		memcpy(group_name,in_buff,FDFS_GROUP_NAME_MAX_LEN);
		group_name[FDFS_GROUP_NAME_MAX_LEN] ='\0';
		if (0 != strcmp(group_name,g_group_name))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, group_name: %s " \
				"not correct, should be: %s", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				group_name, g_group_name);
			resp.status = errno != 0 ? errno : EINVAL;
			break;
		}

		*(in_buff + nInPackLen) = '\0';
		sprintf(full_filename,"%s/data/%s", g_base_path, \
				in_buff+FDFS_GROUP_NAME_MAX_LEN);

		resp.status=getFileContent(full_filename, \
				&file_buff, &file_bytes);
		break;
	}
	resp.cmd = STORAGE_PROTO_CMD_RESP;
	sprintf(resp.pkg_len,"%x",file_bytes);

	if (1 != tcpsenddata(pClientInfo->sock,&resp,sizeof(resp),g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"client ip: %s, send data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pClientInfo->ip_addr, \
			errno, strerror(errno));

		if (NULL != file_buff)
		{
			free(file_buff);
		}
		return errno != 0 ? errno : EPIPE;
	}

	if (0 != resp.status)
	{
		if (NULL != file_buff)
		{
			free(file_buff);
		}
		return resp.status;
	}

	result = tcpsenddata(pClientInfo->sock,file_buff,file_bytes,g_network_timeout);
	if (NULL != file_buff)
	{
		free(file_buff);
	}
	if (1 != result)
	{
		logError("file: %s, line: %d, " \
			"client ip: %s, send data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pClientInfo->ip_addr, \
			errno, strerror(errno));

		return errno != 0 ? errno : EPIPE;
	}

#ifdef __DEBUG__
	printf("storage_download_file done,file:%s,line:%d\n",__FILE__,__LINE__);
#endif

	return resp.status;
}

/**
pkg format:
Header
FDFS_GROUP_NAME_MAX_LEN bytes: group_name
filename
**/
static int storage_delete_file(StorageClientInfo *pClientInfo, \
				const int nInPackLen)
{
	TrackerHeader resp;
	char in_buff[FDFS_GROUP_NAME_MAX_LEN + 32];
	char group_name[FDFS_GROUP_NAME_MAX_LEN + 1];
	char full_filename[MAX_PATH_SIZE+sizeof(in_buff)];
	char meta_filename[MAX_PATH_SIZE+sizeof(in_buff)];
	char *filename;

	while(1)
	{
		if (nInPackLen <= FDFS_GROUP_NAME_MAX_LEN)
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is not correct, " \
				"expect length <= %d", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_UPLOAD_FILE, \
				pClientInfo->ip_addr,  \
				nInPackLen, FDFS_GROUP_NAME_MAX_LEN);
			resp.status = EINVAL;
			break;
		}

		if (nInPackLen >= sizeof(in_buff))
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is too large, " \
				"expect length should < %d", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_UPLOAD_FILE, \
				pClientInfo->ip_addr,  \
				nInPackLen, sizeof(in_buff));
			resp.status = EINVAL;
			break;
		}

		if (1 != tcprecvdata(pClientInfo->sock,in_buff,nInPackLen,g_network_timeout))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, recv data fail, " \
				"errno: %d, error info: %s.", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				errno, strerror(errno));
			resp.status = errno != 0 ? errno : EPIPE;
			break;
		}

		memcpy(group_name,in_buff,FDFS_GROUP_NAME_MAX_LEN);
		group_name[FDFS_GROUP_NAME_MAX_LEN] ='\0';
		if (0 != strcmp(group_name,g_group_name))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, group_name: %s " \
				"not correct, should be: %s", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				group_name, g_group_name);
			resp.status = EINVAL;
			break;
		}

		*(in_buff + nInPackLen) = '\0';
		filename = in_buff + FDFS_GROUP_NAME_MAX_LEN;
		sprintf(full_filename, "%s/data/%s", \
			g_base_path, filename);

		if (0 != unlink(full_filename))
		{
			logError("file: %s, line: %d, " \
				"client ip: %s, delete file %s fail," \
				"errno: %d, error info: %s", \
				__FILE__,__LINE__, pClientInfo->ip_addr, full_filename, \
				errno, strerror(errno));
			resp.status = errno != 0 ? errno : EACCES;
			break;
		}

		resp.status = storage_binlog_write( \
					STORAGE_OP_TYPE_SOURCE_DELETE_FILE, \
					filename);
		if (0 != resp.status)
		{
			break;
		}

		sprintf(meta_filename,"%s"STORAGE_META_FILE_EXT,full_filename);
		if (0 != unlink(meta_filename))
		{
			logError("file: %s, line: %d, " \
				"client ip: %s, delete file %s fail," \
				"errno: %d, error info: %s", \
				__FILE__,__LINE__, \
				pClientInfo->ip_addr, meta_filename, \
				errno, strerror(errno));

			if (errno != ENOENT)
			{
				resp.status = errno != 0 ? errno : EACCES;
				break;
			}
		}

		sprintf(meta_filename,"%s"STORAGE_META_FILE_EXT, \
				filename);
		resp.status = storage_binlog_write( \
					STORAGE_OP_TYPE_SOURCE_DELETE_FILE, \
					meta_filename);
		break;
	}
	resp.cmd = STORAGE_PROTO_CMD_RESP;
	strcpy(resp.pkg_len,"0");

	if (1 != tcpsenddata(pClientInfo->sock,&resp,sizeof(resp),g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"client ip: %s, send data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pClientInfo->ip_addr, \
			errno, strerror(errno));

		return errno != 0 ? errno : EPIPE;
	}

	return resp.status;
}

static int storage_sort_metadata_buff(char *meta_buff, const int meta_size)
{
	FDFSMetaData *meta_list;
	int meta_count;
	int meta_bytes;
	int result;

	meta_list = fdfs_split_metadata(meta_buff, &meta_count, &result);
	if (NULL == meta_list)
	{
		return result;
	}
	qsort((void *)meta_list, meta_count, sizeof(FDFSMetaData), \
		metadata_cmp_by_name);

	fdfs_pack_metadata(meta_list, meta_count, meta_buff, &meta_bytes);

	free(meta_list);

	return 0;
}

static int storage_save_file(StorageClientInfo *pClientInfo, \
			const char *file_buff, const int file_size, \
			char *meta_buff, const int meta_size, \
			char *filename, int *filename_len)
{
	int result;
	int i;
	char full_filename[MAX_PATH_SIZE+32];

	for (i = 0; i < 1024; ++i)
	{
		if (0 != (result = storage_gen_filename(pClientInfo, file_size, \
				filename, filename_len)))
		{
			return result;
		}

		sprintf(full_filename,"%s/data/%s",g_base_path,filename);
		if (!fileExists(full_filename))
		{
			break;
		}

		*full_filename = '\0';
	}

	if ('\0' == *full_filename)
	{
		logError("file: %s, line: %d, " \
			"Can't generate uniq filename", __FILE__,__LINE__);
		*filename = '\0';
		*filename_len = 0;
		return ENOENT;
	}

	if (0 != (result = writeToFile(full_filename, file_buff, file_size)))
	{
		*filename = '\0';
		*filename_len = 0;
		return result;
	}

	if (meta_size > 0)
	{
		char meta_filename[MAX_PATH_SIZE+32];

		if ((result=storage_sort_metadata_buff(meta_buff, \
				meta_size)) != 0)
		{
			*filename = '\0';
			*filename_len = 0;
			unlink(full_filename);
			return result;
		}

		sprintf(meta_filename, "%s"STORAGE_META_FILE_EXT, \
				full_filename);

		if (0 != (result = writeToFile(meta_filename, meta_buff, \
				meta_size)))
		{
			*filename = '\0';
			*filename_len = 0;
			unlink(full_filename);
			return result;
		}
	}

	return 0;
}

/**
9 bytes: meta data bytes
9 bytes: file size 
meta data bytes: each meta data seperated by \x01,
		 name and value seperated by \x02
1 bytes: pad byte, should be \0
file size bytes: file content
**/
static int storage_upload_file(StorageClientInfo *pClientInfo, \
				const int nInPackLen)
{
	TrackerHeader resp;
	int out_len;
	char *in_buff;
	char *meta_buff;
	char out_buff[128];
	char filename[128];
	int meta_bytes;
	int file_bytes;
	int filename_len;

	in_buff = NULL;
	filename[0] = '\0';
	filename_len = 0;

	while(1)
	{
		if (nInPackLen <= 0)
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is not correct, " \
				"expect length > 0", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_UPLOAD_FILE, \
				pClientInfo->ip_addr,  \
				nInPackLen);

			resp.status = EINVAL;
			break;
		}

		in_buff = (char*)malloc(nInPackLen +1);
		if (NULL == in_buff)
		{
			resp.status = errno != 0 ? errno : ENOMEM;
			break;
		}

		if (1 != tcprecvdata(pClientInfo->sock,in_buff,sizeof(in_buff),g_network_timeout))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, recv data fail, " \
				"errno: %d, error info: %s.", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				errno, strerror(errno));
			resp.status = errno != 0 ? errno : EPIPE;
			break;
		}

		*(in_buff + nInPackLen) = '\0';
		meta_bytes = strtol(in_buff,NULL,16);
		file_bytes = strtol(in_buff + TRACKER_PROTO_PKG_LEN_SIZE, \
				NULL, 16);
		if (meta_bytes <0)
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, invalid meta bytes: %d", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				meta_bytes);
			resp.status = EINVAL;
			break;
		}

		if (file_bytes < 0 || file_bytes != nInPackLen - \
			(2 * TRACKER_PROTO_PKG_LEN_SIZE + meta_bytes + 1))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, invalid file bytes: %d", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				file_bytes);
			resp.status = EINVAL;
			break;
		}

		meta_buff = in_buff + 2 * TRACKER_PROTO_PKG_LEN_SIZE;
		*(meta_buff + meta_bytes) = '\0';
		resp.status = storage_save_file(pClientInfo,  \
			meta_buff + meta_bytes + 1, \
			file_bytes, meta_buff, meta_bytes, \
			filename, &filename_len);

		if (0 != resp.status)
		{
			break;
		}

		resp.status = storage_binlog_write( \
					STORAGE_OP_TYPE_SOURCE_CREATE_FILE, \
					filename);
		if (0 != resp.status)
		{
			break;
		}

		if (meta_bytes > 0)
		{
			char meta_filename[64];
			sprintf(meta_filename, "%s"STORAGE_META_FILE_EXT, \
				filename);
			resp.status = storage_binlog_write( \
					STORAGE_OP_TYPE_SOURCE_CREATE_FILE, \
					meta_filename);
		}

		break;
	}
	resp.cmd = STORAGE_PROTO_CMD_RESP;
	if (0 == resp.status)
	{
		out_len = filename_len;
		sprintf(resp.pkg_len, "%x", out_len);

		memcpy(out_buff, &resp, sizeof(resp));
		memcpy(out_buff+sizeof(resp), filename, filename_len);
	}
	else
	{
		out_len = 0;
		sprintf(resp.pkg_len, "%x", out_len);
		memcpy(out_buff, &resp, sizeof(resp));
	}

	if (NULL != in_buff)
	{
		free(in_buff);
	}

	if (1 != tcpsenddata(pClientInfo->sock,out_buff,\
		sizeof(resp) + out_len,g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"client ip: %s, send data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pClientInfo->ip_addr, \
			errno, strerror(errno));
		return errno != 0 ? errno : EPIPE;
	}

	return resp.status;
}

/**
9 bytes: filename bytes
9 bytes: file size
FDFS_GROUP_NAME_MAX_LEN bytes: group_name
filename bytes : filename
file size bytes: file content
**/
static int storage_sync_copy_file(StorageClientInfo *pClientInfo, \
			const int nInPackLen, const char proto_cmd)
{
	TrackerHeader resp;
	char *in_buff;
	char *pBuff;
	char group_name[FDFS_GROUP_NAME_MAX_LEN + 1];
	char filename[128];
	char full_filename[MAX_PATH_SIZE];
	int filename_len;
	int file_bytes;

	in_buff = NULL;
	while(1)
	{
		if (nInPackLen <= 2 * TRACKER_PROTO_PKG_LEN_SIZE + \
					FDFS_GROUP_NAME_MAX_LEN)
		{
			logError("file: %s line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is not correct, " \
				"expect length > %d", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_SYNC_CREATE_FILE, \
				pClientInfo->ip_addr,  nInPackLen, \
				2 * TRACKER_PROTO_PKG_LEN_SIZE + \
					FDFS_GROUP_NAME_MAX_LEN);

			resp.status = EINVAL;
			break;
		}

		in_buff = (char*) malloc(nInPackLen +1);
		if (NULL == in_buff)
		{
			resp.status = errno != 0 ? errno : ENOMEM;
			break;
		}

		if (1 != tcprecvdata(pClientInfo->sock,in_buff,nInPackLen,g_network_timeout))
		{
			logError("file: %s, line: %d, " \
				"client ip: %s, recv data fail, " \
				"expect pkg length: %d, " \
				"errno: %d, error info: %s.", \
				__FILE__,__LINE__, \
				pClientInfo->ip_addr, nInPackLen, \
				errno, strerror(errno));

			resp.status = errno != 0 ? errno : EPIPE;
			break;
		}

		*(in_buff + nInPackLen) = '\0';
		pBuff = in_buff;

		filename_len = strtol(pBuff, NULL, 16);
		pBuff += TRACKER_PROTO_PKG_LEN_SIZE;
		file_bytes = strtol(pBuff, NULL, 16);
		pBuff += TRACKER_PROTO_PKG_LEN_SIZE;

		if (filename_len <= 0 || filename_len >= sizeof(filename))
		{
			logError("file: %s, line: %d, " \
				"client ip: %s, in request pkg, " \
				"filename length: %d is invalid, " \
				"which < 0 or >= %d", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				filename_len,  sizeof(filename));

			resp.status = EPIPE;
			break;
		}

		if (file_bytes < 0)
		{
			logError("file: %s, line: %d, " \
				"client ip: %s, in request pkg, " \
				"file size: %d is invalid, which < 0", \
				__FILE__,__LINE__, pClientInfo->ip_addr, file_bytes);
			resp.status = EPIPE;
			break;
		}

		memcpy(group_name, pBuff, FDFS_GROUP_NAME_MAX_LEN);
		pBuff += FDFS_GROUP_NAME_MAX_LEN;
		group_name[FDFS_GROUP_NAME_MAX_LEN] = '\0';
		if (strcmp(group_name, g_group_name) != 0)
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, group_name: %s " \
				"not correct, should be: %s", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				group_name, g_group_name);
			resp.status = EINVAL;
			break;
		}

		memcpy(filename, pBuff, filename_len);
		filename[filename_len] = '\0';
		snprintf(full_filename, sizeof(full_filename), \
				"%s/data/%s", g_base_path, filename);
		pBuff += filename_len;

		if (file_bytes != (nInPackLen - (pBuff - in_buff)))
		{
			logError("file: %s, line: %d, " \
				"client ip: %s, in request pkg, " \
				"file size: %d != remain bytes: %d", \
				__FILE__,__LINE__, pClientInfo->ip_addr, file_bytes, \
				nInPackLen - (pBuff - in_buff));
			resp.status = EPIPE;
			break;
		}
		if ((proto_cmd == STORAGE_PROTO_CMD_SYNC_CREATE_FILE) && \
			fileExists(full_filename))
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, data file: %s " \
				"already exists, ignore it", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_SYNC_CREATE_FILE, \
				pClientInfo->ip_addr, full_filename);
			resp.status = EEXIST;
			break;
		}

		if (0 != (resp.status = writeToFile(full_filename, \
				pBuff, file_bytes)))
		{
			break;
		}

		if (STORAGE_PROTO_CMD_SYNC_CREATE_FILE == proto_cmd)
		{
			resp.status = storage_binlog_write( \
				STORAGE_OP_TYPE_REPLICA_CREATE_FILE, \
				filename);
		}
		else
		{
			resp.status = storage_binlog_write( \
				STORAGE_OP_TYPE_REPLICA_UPDATE_FILE, \
				filename);
		}
		break;
	}

	if (NULL != in_buff)
	{
		free(in_buff);
	}

	resp.cmd = STORAGE_PROTO_CMD_RESP;
	resp.pkg_len[0] = '0';
	resp.pkg_len[1] = '\0';

	if (1 != tcpsenddata(pClientInfo->sock,&resp,sizeof(resp),g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"client ip: %s, send data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pClientInfo->ip_addr, \
			errno, strerror(errno));
		return errno != 0 ? errno : EPIPE;
	}

	if (resp.status == EEXIST)
	{
		return 0;
	}
	else
	{
		return resp.status;
	}
}

/**
pkg format:
Header
FDFS_GROUP_NAME_MAX_LEN bytes: group_name
filename
**/
static int storage_get_metadata(StorageClientInfo *pClientInfo, \
				const int nInPackLen)
{
	TrackerHeader resp;
	int result;
	char in_buff[FDFS_GROUP_NAME_MAX_LEN + 32];
	char group_name[FDFS_GROUP_NAME_MAX_LEN + 1];
	char full_filename[MAX_PATH_SIZE+sizeof(in_buff)+32];
	char *file_buff;
	int file_bytes;

	file_buff = NULL;
	file_bytes = 0;

	while(1)
	{
		if (nInPackLen <= FDFS_GROUP_NAME_MAX_LEN)
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is not correct, " \
				"expect length <= %d", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_UPLOAD_FILE, \
				pClientInfo->ip_addr,  \
				nInPackLen, FDFS_GROUP_NAME_MAX_LEN);
			resp.status = EINVAL;
			break;
		}

		if (nInPackLen >= sizeof(in_buff))
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is too large, " \
				"expect length should < %d", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_UPLOAD_FILE, \
				pClientInfo->ip_addr,  \
				nInPackLen, sizeof(in_buff));
			resp.status = EINVAL;
			break;
		}

		if (1 != tcprecvdata(pClientInfo->sock,in_buff,nInPackLen,g_network_timeout))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, recv data fail, " \
				"errno: %d, error info: %s.", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				errno, strerror(errno));
			resp.status = errno != 0 ? errno : EPIPE;
			break;
		}

		memcpy(group_name,in_buff,FDFS_GROUP_NAME_MAX_LEN);
		group_name[FDFS_GROUP_NAME_MAX_LEN] ='\0';
		if (0 != strcmp(group_name,g_group_name))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, group_name: %s " \
				"not correct, should be: %s", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				group_name, g_group_name);
			resp.status = EINVAL;
			break;
		}

		*(in_buff + nInPackLen) = '\0';
		sprintf(full_filename, "%s/data/%s", \
				g_base_path, in_buff+FDFS_GROUP_NAME_MAX_LEN);

		if (!fileExists(full_filename))
		{
			resp.status = ENOENT;
			break;
		}

		strcat(full_filename, STORAGE_META_FILE_EXT);
		resp.status = getFileContent(full_filename, \
				&file_buff, &file_bytes);
		if (resp.status == ENOENT)
		{
			resp.status = 0;
		}
		break;
	}
	resp.cmd = STORAGE_PROTO_CMD_RESP;
	sprintf(resp.pkg_len, "%x", file_bytes);

	if (1 != tcpsenddata(pClientInfo->sock,&resp,sizeof(resp),g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"client ip: %s, send data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pClientInfo->ip_addr, \
			errno, strerror(errno));

		if (NULL != file_buff)
		{
			free(file_buff);
		}
		return errno != 0 ? errno : EPIPE;
	}

	if (0 != resp.status)
	{
		if (NULL != file_buff)
		{
			free(file_buff);
		}
		return resp.status;
	}

	result = tcpsenddata(pClientInfo->sock,file_buff,file_bytes,g_network_timeout);
	if (NULL != file_buff)
	{
		free(file_buff);
	}
	if (1 != result)
	{
		logError("file: %s, line: %d, " \
			"client ip: %s, send data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pClientInfo->ip_addr, \
			errno, strerror(errno));

		return errno != 0 ? errno : EPIPE;
	}

	return resp.status;
}

/**
pkg format:
Header
FDFS_GROUP_NAME_MAX_LEN bytes: group_name
filename
**/
static int storage_sync_delete_file(StorageClientInfo *pClientInfo, \
				const int nInPackLen)
{
	TrackerHeader resp;
	char in_buff[FDFS_GROUP_NAME_MAX_LEN + 32];
	char group_name[FDFS_GROUP_NAME_MAX_LEN + 1];
	char full_filename[MAX_PATH_SIZE+sizeof(in_buff)];
	char *filename;

	while(1)
	{
		if (nInPackLen <= FDFS_GROUP_NAME_MAX_LEN)
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is not correct, " \
				"expect length <= %d", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_SYNC_DELETE_FILE, \
				pClientInfo->ip_addr,  \
				nInPackLen, FDFS_GROUP_NAME_MAX_LEN);
			resp.status = EINVAL;
			break;
		}

		if (nInPackLen >= sizeof(in_buff))
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is too large, " \
				"expect length should < %d", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_SYNC_DELETE_FILE, \
				pClientInfo->ip_addr,  \
				nInPackLen, sizeof(in_buff));
			resp.status = EINVAL;
			break;
		}

		if (1 != tcprecvdata(pClientInfo->sock,in_buff,nInPackLen,g_network_timeout))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, recv data fail, " \
				"errno: %d, error info: %s.", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				errno, strerror(errno));
			resp.status = errno != 0 ? errno : EPIPE;
			break;
		}

		memcpy(group_name, in_buff, FDFS_GROUP_NAME_MAX_LEN);
		group_name[FDFS_GROUP_NAME_MAX_LEN] = '\0';
		if (0 != strcmp(group_name,g_group_name))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, group_name: %s " \
				"not correct, should be: %s", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				group_name, g_group_name);
			resp.status = EINVAL;
			break;
		}

		*(in_buff + nInPackLen) = '\0';
		filename = in_buff + FDFS_GROUP_NAME_MAX_LEN;
		sprintf(full_filename, "%s/data/%s", \
			g_base_path, filename);
		if (0 != unlink(full_filename))
		{
			if (ENOENT == errno)
			{
				logError("file: %s, line: %d, " \
					"cmd=%d, client ip: %s, file %s not exist, " \
					"maybe delete later?", \
					__FILE__,__LINE__, \
					STORAGE_PROTO_CMD_SYNC_DELETE_FILE, \
					pClientInfo->ip_addr, full_filename);
			}
			else
			{
				logError("file: %s, line: %d, " \
					"client ip: %s, delete file %s fail," \
					"errno: %d, error info: %s", \
					__FILE__,__LINE__, pClientInfo->ip_addr, \
					full_filename, errno, strerror(errno));
				resp.status = errno != 0 ? errno : EACCES;
				break;
			}
		}
		resp.status = storage_binlog_write( \
					STORAGE_OP_TYPE_REPLICA_DELETE_FILE, \
					filename);
		break;
	}

	resp.cmd = STORAGE_PROTO_CMD_RESP;
	strcpy(resp.pkg_len,"0");

	if (1 != tcpsenddata(pClientInfo->sock,&resp,sizeof(resp),g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"client ip: %s, send data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pClientInfo->ip_addr, \
			errno, strerror(errno));

		return errno != 0 ? errno : EPIPE;
	}

	return resp.status;
}

/**
9 bytes: filename length
9 bytes: meta data size
1 bytes: operation flag, 
     'O' for overwrite all old metadata
     'M' for merge, insert when the meta item not exist, otherwise update it
FDFS_GROUP_NAME_MAX_LEN bytes: group_name
filename
meta data bytes: each meta data seperated by \x01,
		 name and value seperated by \x02
**/
static int storage_set_metadata(StorageClientInfo *pClientInfo, \
				const int nInPackLen)
{

	TrackerHeader resp;
	char *in_buff;
	char group_name[FDFS_GROUP_NAME_MAX_LEN + 1];
	char filename[32];
	char meta_filename[32+sizeof(STORAGE_META_FILE_EXT)];
	char full_filename[MAX_PATH_SIZE + 32 + sizeof(STORAGE_META_FILE_EXT)];
	char op_flag;
	char sync_flag;
	char *meta_buff;
	int meta_bytes;
	int filename_len;

	in_buff = NULL;

	while(1)
	{
		if (nInPackLen <= 2 * TRACKER_PROTO_PKG_LEN_SIZE + 1 + \
					FDFS_GROUP_NAME_MAX_LEN)
		{
			logError("file: %s, line: %d, " \
				"cmd=%d, client ip: %s, package size %d " \
				"is not correct, " \
				"expect length > %d", \
				__FILE__,__LINE__, \
				STORAGE_PROTO_CMD_SET_METADATA, \
				pClientInfo->ip_addr,  \
				nInPackLen, 2 * TRACKER_PROTO_PKG_LEN_SIZE + 1 \
				+ FDFS_GROUP_NAME_MAX_LEN);
			resp.status = EINVAL;
			break;
		}

		in_buff = (char*)malloc(nInPackLen +1);
		if (NULL == in_buff)
		{
			resp.status = errno != 0 ? errno : ENOMEM;
			break;
		}

		if (1 != tcprecvdata(pClientInfo->sock,in_buff,nInPackLen,g_network_timeout))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, recv data fail, " \
				"errno: %d, error info: %s.", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				errno, strerror(errno));
			resp.status = errno != 0 ? errno : EPIPE;
			break;
		}

		*(in_buff + nInPackLen) = '\0';
		filename_len = strtol(in_buff,NULL,16);
		meta_bytes = strtol(in_buff + TRACKER_PROTO_PKG_LEN_SIZE, \
				NULL, 16);

		if (filename_len <=0 || filename_len >= sizeof(filename))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, invalid filename length: %d", \
				__FILE__,__LINE__, pClientInfo->ip_addr, filename_len);
			resp.status = EINVAL;
			break;
		}

		op_flag = *(in_buff + 2 * TRACKER_PROTO_PKG_LEN_SIZE);
		if (op_flag != STORAGE_SET_METADATA_FLAG_OVERWRITE && \
			op_flag != STORAGE_SET_METADATA_FLAG_MERGE)
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, " \
				"invalid operation flag: 0x%02X", \
				__FILE__,__LINE__, pClientInfo->ip_addr, op_flag);
			resp.status = EINVAL;
			break;
		}

		if (meta_bytes < 0 || meta_bytes != nInPackLen - \
				(2 * TRACKER_PROTO_PKG_LEN_SIZE + 1 + \
				FDFS_GROUP_NAME_MAX_LEN + filename_len))
		{
			logError("file: "__FILE__", line: %d, " \
				"client ip:%s, invalid meta bytes: %d", \
				__LINE__, pClientInfo->ip_addr, meta_bytes);
			resp.status = EINVAL;
			break;
		}

		memcpy(group_name,in_buff + 2* TRACKER_PROTO_PKG_LEN_SIZE +1,\
			FDFS_GROUP_NAME_MAX_LEN);
		group_name[FDFS_GROUP_NAME_MAX_LEN] ='\0';

		if (0 != strcmp(group_name,g_group_name))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, group_name: %s " \
				"not correct, should be: %s", \
				__FILE__,__LINE__, pClientInfo->ip_addr, \
				group_name, g_group_name);
			resp.status = EINVAL;
			break;
		}

		memcpy(filename, in_buff + 2 * TRACKER_PROTO_PKG_LEN_SIZE + 1 + \
			FDFS_GROUP_NAME_MAX_LEN, filename_len);
		*(filename + filename_len) = '\0';

		meta_buff = in_buff + 2 * TRACKER_PROTO_PKG_LEN_SIZE + 1 + \
				FDFS_GROUP_NAME_MAX_LEN + filename_len;
		*(meta_buff + meta_bytes) = '\0';

		sprintf(full_filename,"%s/data/%s",g_base_path,filename);
		if (!fileExists(full_filename))
		{
			logError("file: %s, line: %d, " \
				"client ip:%s, filename: %s not exist", \
				__FILE__,__LINE__, pClientInfo->ip_addr, filename);
			resp.status = ENOENT;
			break;
		}

		sprintf(meta_filename, "%s"STORAGE_META_FILE_EXT, filename);
		strcat(full_filename, STORAGE_META_FILE_EXT);

		resp.status = storage_do_set_metadata(pClientInfo, \
			full_filename, meta_buff, meta_bytes, \
			op_flag, &sync_flag);

		if (0 != resp.status)
		{
			break;
		}
		if (0 != sync_flag)
		{
			resp.status = storage_binlog_write( \
					sync_flag, meta_filename);
		}
		break;

	}
	resp.cmd = STORAGE_PROTO_CMD_RESP;
	sprintf(resp.pkg_len, "%x", 0);

	if (NULL != in_buff)
	{
		free(in_buff);
	}

	if (1 != tcpsenddata(pClientInfo->sock,(void *)&resp,sizeof(resp),g_network_timeout))
	{
		logError("file: %s, line: %d, " \
			"client ip: %s, send data fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, pClientInfo->ip_addr, \
			errno, strerror(errno));
		return errno != 0 ? errno : EPIPE;
	}

	return resp.status;
}

void* storage_thread_entrance(void* arg)
{
/*
package format:
9 bytes length (hex string)
1 bytes cmd (char)
1 bytes status(char)
data buff (struct)
*/

	StorageClientInfo client_info;
	TrackerHeader header;
	int result;
	int nInPackLen;
	int count;

	memset(&client_info,0,sizeof(client_info));
	client_info.sock = (int) arg;

	getPeerIpaddr(client_info.sock,client_info.ip_addr,FDFS_IPADDR_SIZE);

	count =0;
	while(g_continue_flag)
	{

		result = tcprecvdata(client_info.sock,&header,sizeof(header),g_network_timeout);
		if (0 == result)
		{
			continue;
		}
		if (1 != result)
		{
			logError("file: %s, line: %d, " \
				"client ip: %s, recv data fail, " \
				"errno: %d, error info: %s.", \
				__FILE__,__LINE__, client_info.ip_addr, \
				errno, strerror(errno));
			break;
		}

		header.pkg_len[sizeof(header.pkg_len)-1] = '\0';
		nInPackLen = strtol(header.pkg_len, NULL, 16);

		if (STORAGE_PROTO_CMD_DOWNLOAD_FILE == header.cmd)
		{
			g_storage_stat.total_download_count++;
			if (0 != storage_download_file(&client_info, \
				nInPackLen))
			{
				break;
			}

			g_storage_stat.success_download_count++;
			CHECK_AND_WRITE_TO_STAT_FILE
		}
		else if (STORAGE_PROTO_CMD_GET_METADATA == header.cmd)
		{
			g_storage_stat.total_get_meta_count++;
			if (0 != storage_get_metadata(&client_info, \
				nInPackLen))
			{
				break;
			}
			g_storage_stat.success_get_meta_count++;
			CHECK_AND_WRITE_TO_STAT_FILE
		}
		else if (STORAGE_PROTO_CMD_UPLOAD_FILE == header.cmd)
		{
			g_storage_stat.total_upload_count++;
			if (0 != storage_upload_file(&client_info, \
				nInPackLen))
			{
				break;
			}
			g_storage_stat.success_upload_count++;
			g_storage_stat.last_source_update = time(NULL);
			CHECK_AND_WRITE_TO_STAT_FILE
		}
		else if (STORAGE_PROTO_CMD_DELETE_FILE == header.cmd)
		{
			g_storage_stat.total_delete_count++;
			if (0 != storage_delete_file(&client_info, \
				nInPackLen))
			{
				break;
			}
			g_storage_stat.success_delete_count++;
			g_storage_stat.last_source_update = time(NULL);
			CHECK_AND_WRITE_TO_STAT_FILE
		}
		else if (STORAGE_PROTO_CMD_SYNC_CREATE_FILE == header.cmd)
		{
			if (0 != storage_sync_copy_file(&client_info, \
				nInPackLen, header.cmd))
			{
				break;
			}
			g_storage_stat.last_sync_update = time(NULL);
			CHECK_AND_WRITE_TO_STAT_FILE
		}
		else if (STORAGE_PROTO_CMD_SYNC_DELETE_FILE == header.cmd)
		{
			if (0 != storage_sync_delete_file(&client_info, \
				nInPackLen))
			{
				break;
			}
			g_storage_stat.last_sync_update = time(NULL);
			CHECK_AND_WRITE_TO_STAT_FILE
		}
		else if (STORAGE_PROTO_CMD_SYNC_UPDATE_FILE == header.cmd)
		{
			if (0 != storage_sync_copy_file(&client_info, \
				nInPackLen, header.cmd))
			{
				break;
			}
			g_storage_stat.last_sync_update = time(NULL);
			CHECK_AND_WRITE_TO_STAT_FILE
		}
		else if (STORAGE_PROTO_CMD_SET_METADATA == header.cmd)
		{
			g_storage_stat.total_set_meta_count++;
			if (0 != storage_set_metadata(&client_info, \
				nInPackLen))
			{
				break;
			}
			g_storage_stat.success_set_meta_count++;
			g_storage_stat.last_source_update = time(NULL);
			CHECK_AND_WRITE_TO_STAT_FILE
		}
		else if (TRACKER_PROTO_CMD_STORAGE_QUIT == header.cmd)
		{

#ifdef __DEBUG__
			printf("TRACKER_PROTO_CMD_STORAGE_QUIT file:%s,line:%d\n",__FILE__,__LINE__);
#endif
			break;
		}
		else
		{
			logError("file: %s, line: %d, "   \
				"client ip: %s, unkown cmd: %d", \
				__FILE__,__LINE__, client_info.ip_addr, header.cmd);
			break;
		}
		count++;
	}

	if (0 != pthread_mutex_lock(&g_storage_thread_lock))
	{
		logError("file: %s, line: %d, " \
			"call pthread_mutex_lock fail, " \
			"errno: %d, error info:%s.", \
			__FILE__,__LINE__, errno, strerror(errno));
	}
	g_storage_thread_count--;

#ifdef __DEBUG__
			printf("g_storage_thread_count:%d, file:%s,line:%d\n",\
				g_storage_thread_count,__FILE__,__LINE__);
#endif


	if (0 != pthread_mutex_unlock(&g_storage_thread_lock))
	{
		logError("file: %s, line: %d, " \
			"call pthread_mutex_unlock fail, " \
			"errno: %d, error info:%s.", \
			__FILE__,__LINE__, errno, strerror(errno));
	}
	close(client_info.sock);

#ifdef __DEBUG__
	printf("storage_thread_entrance done! file :%s,line:%d, to be finish =====>\n",\
			__FILE__,__LINE__);
#endif

	return NULL;
}