#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include "sockopt.h"
#include "logger.h"

int socketServer(const char *bind_ipaddr, const int port, \
		const char *szLogFilePrefix)
{

	struct sockaddr_in bindaddr;
	int sock;
	int result;

	sock = socket(AF_INET,SOCK_STREAM,0);
	if (sock <0)
	{
		logErrorEx(szLogFilePrefix,"file: "__FILE__",line:%d"\
			"socket creat file,errno= %d,err info= %s",\
			__LINE__,errno,strerror(errno));
		return -1;
	}

	result = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &result, sizeof(int));
	if (result <0)
	{
		logErrorEx(szLogFilePrefix,"file: "__FILE__",line:%d"\
			"setsockopt failed,errno= %d,err info= %s",\
			__LINE__,errno,strerror(errno));
		close(sock);
		return -2;
	}

	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port = htons(port);
	if (NULL == bind_ipaddr || '\0' == bind_ipaddr[0])
	{
		bindaddr.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		if (0 == inet_aton(bind_ipaddr,&bindaddr.sin_addr))
		{
			logErrorEx(szLogFilePrefix,"file: "__FILE__",line:%d,"\
			"invalid ip %s,errno = %d ,err info = %s",\
			__LINE__,bind_ipaddr,errno,strerror(errno));

			close(sock);
			return -3;
		}
	}

	result = bind(sock,(struct sockaddr*)&bindaddr, sizeof(bindaddr));
	if (result <0)
	{
		logErrorEx(szLogFilePrefix,"file: "__FILE__",line:%d,"\
			"bind port fail,errno = %d,err info = %s",\
			__LINE__,errno,strerror(errno));

		close(sock);
		return -4;
	}

	result = listen(sock,5);
	if (result <0)
	{
		logErrorEx(szLogFilePrefix,"file: "__FILE__",line:%d,"\
			"listen port fail,errno = %d,err info = %s",\
			__LINE__,errno,strerror(errno));

		close(sock);
		return -5;
	}
	return sock;

#ifdef __DEBUG__
	printf("listen...!,socketServer done\n");
#endif
}

in_addr_t getIpaddrByName(const char *name, char *buff, const int bufferSize)
{
    	struct in_addr ip_addr;
	struct hostent *ent;
	in_addr_t **addr_list;

	if (inet_pton(AF_INET, name, &ip_addr) == 1)
	{
		snprintf(buff, bufferSize, "%s", name);
		return ip_addr.s_addr;
	}

	ent = gethostbyname(name);
	if (ent == NULL)
	{
		return INADDR_NONE;
	}
        addr_list = (in_addr_t **)ent->h_addr_list;
	if (addr_list[0] == NULL)
	{
		return INADDR_NONE;
	}

	memset(&ip_addr, 0, sizeof(ip_addr));
	ip_addr.s_addr = *(addr_list[0]);
	snprintf(buff, bufferSize, "%s", inet_ntoa(ip_addr));
	return ip_addr.s_addr;
}

int nbaccept(int sock,int timeout,int *err_no)
{

	struct sockaddr_in inaddr;
	unsigned int sockaddr_len;
	fd_set read_set;
	fd_set exception_set;
	struct timeval t;
	int result;

	if (timeout > 0)
	{
		t.tv_sec =timeout;
		t.tv_usec = 0;

		FD_ZERO(&read_set);
		FD_ZERO(&exception_set);
		FD_SET(sock,&read_set);
		FD_SET(sock,&exception_set);

		result = select(sock+1,&read_set,NULL,&exception_set,&t);
		if (0 == result)
		{
			*err_no = ETIMEDOUT;
			return -1;
		}
		else if (result <0)
		{
			*err_no = errno;
			return -1;
		}
		if (!FD_ISSET(sock,&read_set))
		{
			*err_no = EAGAIN;
			return -1;
		}
	}
	sockaddr_len = sizeof(inaddr);
	result = accept(sock,(struct sockaddr *)&inaddr,&sockaddr_len);
	if (result <0)
	{
		*err_no = errno;
	}
	else
	{
		*err_no = 0;
	}

#ifdef __DEBUG__
	printf("nbaccept done\n");
#endif
	return result;
}