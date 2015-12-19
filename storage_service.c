#include <stdio.h>
#include "storage_service.h"

void* storage_thread_entrance(void* arg)
{
/*
package format:
9 bytes length (hex string)
1 bytes cmd (char)
1 bytes status(char)
data buff (struct)
*/

#ifdef __DEBUG__
	printf("storage_thread_entrance done! file :%s,line:%d, to be finish =====>\n",\
			__FILE__,__LINE__);
#endif

	return NULL;
}