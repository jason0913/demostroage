#ifndef __INI_FILE_READER_H
#define __INI_FILE_READER_H

#include "fdfs_define.h"

#define INI_ITEM_NAME_LEN		64
#define INI_ITEM_VALUE_LEN		128

typedef struct
{
	char name[INI_ITEM_NAME_LEN + 1];
	char value[INI_ITEM_VALUE_LEN + 1];
} IniItemInfo;

extern int iniLoadItems(const char *szFilename, IniItemInfo **ppItems, \
			int *nItemCount);

extern void iniFreeItems(IniItemInfo *items);

extern char *iniGetStrValue(const char *szName, IniItemInfo *items, \
			const int nItemCount);
extern int iniGetIntValue(const char *szName, IniItemInfo *items, \
			const int nItemCount, const int nDefaultValue);
extern bool iniGetBoolValue(const char *szName, IniItemInfo *items, \
		const int nItemCount);
extern int iniGetValues(const char *szName, IniItemInfo *items, const int nItemCount, \
			char **szValues, const int max_values);
#endif