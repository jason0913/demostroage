#ifndef __STORAGE_FUNC_
#define __STORAGE_FUNC_

extern int storage_load_from_conf_file(const char *conf_filename,\
			char *bind_addr,const int addr_size);
extern int storage_check_and_make_data_dirs();
extern int storage_open_storage_stat();
extern int storage_close_storage_stat();

#endif