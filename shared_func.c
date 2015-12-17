#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include "storage_func.h"
#include "fdfs_define.h"
#include "logger.h"

void daemon_init(bool bCloseFiles)
{
	printf("daemon_init done\n");
	return;
}

int init_pthread_lock(pthread_mutex_t *pthread_lock)
{

	pthread_mutexattr_t mat;

	if(0 != pthread_mutexattr_init(&mat))
	{
		logError("file:%s,line:%d," \
			"call pthread_mutexattr_t fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,errno,strerror(errno));

		return errno != 0? errno:EAGAIN;

	}

	if (0 != pthread_mutexattr_settype(&mat,PTHREAD_MUTEX_ERRORCHECK))
	{
		logError("file:%s,line:%d," \
			"call pthread_mutexattr_settype fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,errno,strerror(errno));

		return errno != 0? errno:EAGAIN;
	}

	if (0 != pthread_mutex_init(pthread_lock,&mat))
	{
		logError("file:%s,line:%d," \
			"call pthread_mutex_init fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,errno,strerror(errno));

		return errno != 0? errno:EAGAIN;
	}

	if (0 != pthread_mutexattr_destroy(&mat))
	{
		logError("file:%s,line:%d," \
			"call pthread_mutexattr_destroy fail ,errno = %d,err info = %s",
			__FILE__,__LINE__,errno,strerror(errno));

		return errno != 0? errno:EAGAIN;
	}

#ifdef __DEBUG__
	printf("init_pthread_lock done\n");
#endif

	return 0;
}

bool fileExists(const char *filename)
{
	return access(filename,0) ==0;
}

bool isDir(const char *filename)
{
	struct stat buf;
	if (0 != stat(filename,&buf))
	{
		return false;
	}
	return S_ISREG(buf.st_mode);
}