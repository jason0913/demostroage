#ifndef __INI_FILE_READER_H
#define __INI_FILE_READER_H

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

#endif