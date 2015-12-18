#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ini_file_reader.h"

#define _LINE_BUFFER_SIZE	512
#define _ALLOC_ITEMS_ONCE	  8

int compareByItemName(const void *p1, const void *p2)
{
	return strcmp(((IniItemInfo *)p1)->name, ((IniItemInfo *)p2)->name);
}

int iniLoadItems(const char *szFilename, IniItemInfo **ppItems, int *nItemCount)
{

	FILE *fp;
	IniItemInfo *items;
	IniItemInfo *pItem;
	int alloc_items;
	char szLineBuff[_LINE_BUFFER_SIZE];
	char *pEqualChar;
	int nNameLen;
	int nValueLen;

	alloc_items = _ALLOC_ITEMS_ONCE;
	items = (IniItemInfo *)malloc(alloc_items *sizeof(IniItemInfo));
	if (NULL == items)
	{
		*nItemCount = -1;
		*ppItems = NULL;
		return errno != 0? errno:ENOMEM;
	}

	if (NULL == (fp=fopen(szFilename,"r")))
	{
		fclose(fp);
		*nItemCount = -1;
		*ppItems = NULL;
		return errno != 0? errno:ENOMEM;
	}

	memset(items,0,alloc_items * sizeof(IniItemInfo));
	memset(szLineBuff,0,sizeof(szLineBuff));

	pItem = items;
	*nItemCount = 0;

	while(1)
	{
		if (NULL == fgets(szLineBuff,_LINE_BUFFER_SIZE,fp))
		{
			break;
		}
		trim(szLineBuff);
		if ('#' == szLineBuff[0] || '\0' == szLineBuff[0])
		{
			continue;
		}

		pEqualChar = strchr(szLineBuff,'=');
		if (NULL == pEqualChar)
		{
			continue;
		}

		nNameLen = pEqualChar - szLineBuff;
		nValueLen = strlen(szLineBuff) -(nNameLen +1);

		if (nNameLen > INI_ITEM_NAME_LEN)
		{
			nNameLen = INI_ITEM_NAME_LEN;
		}
		if (nValueLen > INI_ITEM_VALUE_LEN)
		{
			nValueLen = INI_ITEM_VALUE_LEN;
		}

		if (*nItemCount >= alloc_items)
		{
			alloc_items += _ALLOC_ITEMS_ONCE;
			items = (IniItemInfo*) realloc(items, alloc_items * sizeof(IniItemInfo));
			if (NULL == items)
			{
				fclose(fp);
				*nItemCount = -1;
				*ppItems = NULL;
				return errno != 0? errno:ENOMEM;
			}
			pItem = items + *nItemCount;
			memset(pItem, 0, sizeof(IniItemInfo) * \
							(alloc_items - (*nItemCount)));
		}

		memcpy(pItem->name,szLineBuff,nNameLen);
		memcpy(pItem->value,pEqualChar + 1,nValueLen);

		trim(pItem->name);
		trim(pItem->value);

		(*nItemCount)++;
		pItem++;
	}
	fclose(fp);
	qsort(items, *nItemCount, sizeof(IniItemInfo), compareByItemName);

	*ppItems = items;
	return 0;
}

void iniFreeItems(IniItemInfo *items)
{
	;
}