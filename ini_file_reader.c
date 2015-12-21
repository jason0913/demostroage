#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ini_file_reader.h"
#include "shared_func.h"

#define _LINE_BUFFER_SIZE	512
#define _ALLOC_ITEMS_ONCE	  8

int compareByItemName(const void *p1, const void *p2)
{
	return strcmp(((IniItemInfo *)p1)->name, ((IniItemInfo *)p2)->name);
}

char *iniGetStrValue(const char *szName, IniItemInfo *items, \
			const int nItemCount)
{
	IniItemInfo targetItem;
	void *pResult;

	if (nItemCount <=0)
	{
		return NULL;
	}
	snprintf(targetItem.name,sizeof(targetItem.name),"%s",szName);
	pResult = bsearch(&targetItem,items,nItemCount,sizeof(IniItemInfo),compareByItemName);
	if (NULL == pResult)
	{
		return NULL;
	}
	else
		return ((IniItemInfo*)pResult)->value;
}

int iniGetIntValue(const char *szName, IniItemInfo *items, \
			const int nItemCount, const int nDefaultValue)
{
	char *pValue;

	pValue = iniGetStrValue(szName,items,nItemCount);
	if (NULL == pValue)
	{
		return nDefaultValue;
	}
	else
		return atoi(pValue);
}

bool iniGetBoolValue(const char *szName, IniItemInfo *items, \
		const int nItemCount)
{
	char *pValue;

	pValue = iniGetStrValue(szName,items,nItemCount);
	if (NULL == pValue)
	{
		return false;
	}
	else
	{
		return strcasecmp(pValue,"true") ==0 ||
				strcasecmp(pValue,"yes") ==0 ||
				strcasecmp(pValue,"on") ==0||
				strcmp(pValue,"1") ==0;
	}
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
	if (NULL != items)
	{
		free(items);
	}
}

int iniGetValues(const char *szName, IniItemInfo *items, const int nItemCount, \
			char **szValues, const int max_values)
{
	IniItemInfo targetItem;
	IniItemInfo *pFound;
	IniItemInfo *pItem;
	IniItemInfo *pItemEnd;

	char **ppValues;

	if (nItemCount <= 0 || max_values <=0)
	{
		return 0;
	}
	snprintf(targetItem.name,sizeof(targetItem.name),"%s",szName);
	pFound = (IniItemInfo *)bsearch(&targetItem, items, nItemCount, \
				sizeof(IniItemInfo), compareByItemName);
	if (NULL == pFound)
	{
		return 0;
	}

	ppValues = szValues;
	*ppValues++ = pFound->value;
	for (pItem = pFound -1; pItem >= items;pItem--)
	{
		if (strcmp(pItem->name,szName) != 0)
		{
			break;
		}
		if (ppValues - szValues < max_values)
		{
			*ppValues++ = pItem->value;
		}
	}

	pItemEnd = items + nItemCount;
	for (pItem = pFound +1; pItem < pItemEnd; pItem++)
	{
		if (0 != strcmp(pItem->name, szName))
		{
			break;
		}
		if (ppValues - szValues < max_values)
		{
			*ppValues++ = pItem->value;
		}
	}
	return ppValues - szValues;
}
