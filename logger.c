#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "logger.h"
//#include "fdfs_define.h"
#include "fdfs_global.h"

int check_and_mk_log_dir()
{
	printf("check_and_mk_log_dir done!\n");

	return 0;
}
static void doLog(const char *prefix,const char *text)
{
	time_t t;
	struct tm *pCurrentTime;
	char datebuffer[32];
	char logfile[MAX_PATH_SIZE];
	FILE *fp;
	int fd;
	struct flock lock;

	t = time(NULL);
	pCurrentTime = localtime(&t);
	strftime(datebuffer,sizeof(datebuffer),"[%Y-%m-%d %X]",pCurrentTime);

	if (0 != prefix)
	{
		snprintf(logfile,sizeof(logfile),"%s/logs/%s.log",g_base_path,prefix);
		umask(0);
		if (NULL == fopen(logfile,"a"))
		{
			fp = stderr;
		}
	}
	else
		fp = stderr;

	fd = fileno(fp);

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 10;

	if (0 == fcntl(fd,F_SETLKW, &lock))
	{
		fprintf(fp, "%s %s\n", datebuffer,text);
	}

	lock.l_type = F_UNLCK;
	fcntl(fd,F_SETLKW, &lock);
	if (fp != stderr)
	{
		fclose(fp);
	}
}
void logError(const char *prefix, const char *format,...)
{
	char logBuffer[LINE_MAX];
	va_list ap;
	va_start(ap,format);
	vsnprintf(logBuffer,sizeof(logBuffer),format,ap);
	doLog(prefix,logBuffer);
	va_end(ap);
}