#ifndef __LOGGER_H_
#define __LOGGER_H_

extern int check_and_mk_log_dir();

extern void logError(const char *prefix, const char *format,...);
extern void logInfo(const char *format,...);

#endif