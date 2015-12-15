#ifndef __SOCKOPT_H
#define __SOCKOPT_H

extern int socketServer(const char *bind_ipaddr, const int port, \
		const char *szLogFilePrefix);
extern int nbaccept(int sock,int timeout,int *err_no);

#endif