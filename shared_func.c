#include <stdio.h>
#include <stdlib.h>
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

	pid_t pid;
	int i;

	if (0 != (pid = fork()))
	{
		exit(0);
	}
	setsid();
	if((pid=fork())!=0)
	{
		exit(0);
	}
	chdir("/");

	if (true == bCloseFiles)
	{
		for (i = 0; i < 64; ++i)
		{
			close(i);
		}
	}
#ifdef __DEBUG__
	printf("daemon_init done\n");
#endif
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

void chopPath(char *filePath)
{
	int lastIndex;
	if ('\0' == filePath)
	{
		return;
	}
	lastIndex = strlen(filePath)-1;
	if (filePath[lastIndex] == '/')
	{
		*(filePath +lastIndex) = '\0';
	}

}
bool isDir(const char *filename)
{
	struct stat buf;
	if (0 != stat(filename,&buf))
	{
		return false;
	}
	return S_ISDIR(buf.st_mode);
}

char * trim_right(char *pStr)
{
	int len;
	char *pEnd;
	char *p;
	char ch;

	len = strlen(pStr);
	if (0 == len)
	{
		return pStr;
	}
	pEnd = pStr + strlen(pStr) -1;
	for (p = pEnd; p >= pStr; --p)
	{
		ch = *p;
		if (!(' ' == ch || '\n' == ch || '\r' == ch || '\t' == ch))
		{
			break;
		}
	}
	if (p != pEnd)
	{
		*(p +1) = '\0';
	}
	return pStr;
}

char * trim_left(char *pStr)
{
	int iLength;
	char *pTemp;
	char ch;
	int nDestlen;

	iLength = strlen(pStr);
	int i;
	for (i =0; i < iLength; i++)
	{
		ch = pStr[i];
		if (!(' ' == ch || '\n' == ch || '\r' == ch || '\t' == ch))
		{
			break;
		}
	}
	if (i == 0)
	{
		return pStr;
	}
	nDestlen = iLength - i;
	pTemp = (char *)malloc(nDestlen + 1);
	strcpy(pTemp, pStr + i);
	strcpy(pStr, pTemp);
	free(pTemp);
	return pStr;
}

char *trim(char *pStr)
{
	trim_right(pStr);
	trim_left(pStr);
	return pStr;
}

void int2buff(const int n, char *buff)
{
	unsigned char *p;
	p = (unsigned char *)buff;
	*p++ = (n >> 24) & 0xFF;
	*p++ = (n >> 16) & 0xFF;
	*p++ = (n >> 8) & 0xFF;
	*p++ = n & 0xFF;
}

int getFileContent(const char *filename, char **buff, int *file_size)
{

	FILE *fp;

	fp = fopen(filename,"rb");
	if (NULL == fp)
	{
		*buff = NULL;
		*file_size = 0;
		return errno != 0 ? errno : ENOENT;
	}

	if (0 != fseek(fp,0,SEEK_END))
	{
		*buff = NULL;
		*file_size = 0;
		fclose(fp);
		return errno != 0 ? errno : ENOENT;
	}

	*file_size = ftell(fp);
	if (*file_size < 0)
	{
		*buff = NULL;
		*file_size = 0;
		fclose(fp);
		return errno != 0 ? errno : ENOENT;
	}

	*buff = (char *) malloc(*file_size +1);
	if (NULL == *buff)
	{
		*file_size = 0;
		fclose(fp);
		return errno != 0 ? errno : ENOMEM;
	}

	rewind(fp);
	if (fread(*buff,1,*file_size,fp) != *file_size)
	{
		free(*buff);
		*buff = NULL;
		*file_size = 0;
		fclose(fp);
		return errno != 0 ? errno : ENOENT;
	}
	(*buff)[*file_size] ='\0';
	fclose(fp);

	return 0;
}

int writeToFile(const char *filename, const char *buff, const int file_size)
{
	FILE *fp;
	fp = fopen(filename,"wb");
	if (NULL == fp)
	{
		logError("file: %s, line: %d, " \
			"open file %s fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, filename, \
			errno, strerror(errno));
		return errno != 0 ? errno : ENOENT;
	}

	if (file_size != fwrite(buff,1,file_size,fp))
	{
		logError("file: %s, line: %d, " \
			"write file %s fail, " \
			"errno: %d, error info: %s", \
			__FILE__,__LINE__, filename, \
			errno, strerror(errno));
		fclose(fp);
		return errno != 0 ? errno : ENOENT;
	}

	fclose(fp);
	return 0;
}

int getOccurCount(const char *src, const char seperator)
{
	int count;
	char *p;

	count =0;
	p = strchr(src,seperator);
	while(NULL != p)
	{
		count++;
		p = strchr(p+1,seperator);
	}

	return count;
}

int splitEx(char *src, const char seperator, char **pCols, const int nMaxCols)
{
	char *p;
	char **pCurrent;
	int count = 0;

	if (nMaxCols <= 0)
	{
		return 0;
	}

	p = src;
	pCurrent = pCols;

	while(1)
	{
		*pCurrent = p;
		pCurrent ++;

		count++;
		if (count >= nMaxCols)
		{
			break;
		}

		p = strchr(p,seperator);
		if (NULL == p)
		{
			break;
		}
		*p = '\0';
		p++;
	}

	return count;
}