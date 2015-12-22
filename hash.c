#include "hash.h"

// P.J.Weinberger Hash Function
unsigned int PJWHash(const void *key, const int key_len)
{
    unsigned char *pKey;
    unsigned char *pEnd;
    unsigned int BitsInUnignedInt = (unsigned int)(sizeof(unsigned int) * 8);
    unsigned int ThreeQuarters    = (unsigned int)((BitsInUnignedInt * 3) / 4);
    unsigned int OneEighth        = (unsigned int)(BitsInUnignedInt / 8);

    unsigned int HighBits         = (unsigned int)(0xFFFFFFFF) << (BitsInUnignedInt - OneEighth);
    unsigned int hash             = 0;
    unsigned int test             = 0;

    pEnd = (unsigned char *)key + key_len;
    for (pKey = (unsigned char *)key; pKey != pEnd; pKey++)
    {
        hash = (hash << OneEighth) + (*(pKey));
        if ((test = hash & HighBits) != 0)
        {
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
        }
    }

    return hash;
}