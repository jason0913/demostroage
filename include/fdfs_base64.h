#ifndef __FDFS_BASE64_H
#define __FDFS_BASE64_H

#include "fdfs_define.h"

extern char *base64_encode_ex(char *src, const int nSrcLen, char *dest, \
                       int *dest_len, const bool bPad);
extern char *base64_decode(char *src, const int nSrcLen, char *dest, int *dest_len);

#endif