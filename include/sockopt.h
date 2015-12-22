#ifndef __SOCKOPT_H
#define __SOCKOPT_H

#include <arpa/inet.h>

typedef int (*getnamefunc)(int socket, struct sockaddr *address, socklen_t *address_len);

#define getSockIpaddr(sock, buff, bufferSize) getIpaddr(getsockname, sock, buff, bufferSize)
#define getPeerIpaddr(sock, buff, bufferSize) getIpaddr(getpeername, sock, buff, bufferSize)

extern int socketServer(const char *bind_ipaddr, const int port, \
		const char *szLogFilePrefix);
extern int nbaccept(int sock,int timeout,int *err_no);

extern in_addr_t getIpaddr(getnamefunc getname, int sock, char *buff, const int bufferSize);
extern in_addr_t getIpaddrByName(const char *name, char *buff, const int bufferSize);

extern int tcprecvdata(int sock,void* data,int size,int timeout);
extern int tcpsenddata(int sock,void* data,int size,int timeout);

extern int connectserverbyip(int sock, char* ip, short port);

#endif