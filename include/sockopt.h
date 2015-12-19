#ifndef __SOCKOPT_H
#define __SOCKOPT_H

#include <arpa/inet.h>

extern int socketServer(const char *bind_ipaddr, const int port, \
		const char *szLogFilePrefix);
extern int nbaccept(int sock,int timeout,int *err_no);

extern in_addr_t getIpaddrByName(const char *name, char *buff, const int bufferSize);
#endif