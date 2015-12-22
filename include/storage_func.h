#ifndef __STORAGE_FUNC_
#define __STORAGE_FUNC_

#define STORAGE_DATA_DIR_FORMAT		"%02X"
#define STORAGE_META_FILE_EXT		"-m"

typedef char * (*get_filename_func)(const void *pArg, \
			char *full_filename);

extern int storage_write_to_fd(int fd, get_filename_func filename_func, \
		const void *pArg, const char *buff, const int len);

extern int storage_load_from_conf_file(const char *conf_filename,\
			char *bind_addr,const int addr_size);

extern int storage_check_and_make_data_dirs();
extern int storage_open_storage_stat();
extern int storage_close_storage_stat();
extern int storage_write_to_stat_file();

extern int storage_write_to_sync_ini_file();

#endif