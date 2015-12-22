#include <string.h>

#include "fdfs_base64.h"

static char line_separator[16];
static int line_sep_len;

/**
* max chars per line, excluding line_separator.  A multiple of 4.
*/
static int line_length = 72;

/**
* letter of the alphabet used to encode binary values 0..63
*/
static unsigned char valueToChar[64];

/**
* binary value encoded by a given letter of the alphabet 0..63
*/
static int charToValue[256];
static int pad_ch;


char *base64_encode_ex(char *src, const int nSrcLen, char *dest, \
                       int *dest_len, const bool bPad)
{
  int linePos;
  int leftover;
  int combined;
  char *pDest;
  int c0, c1, c2, c3;
  unsigned char *pRaw;
  unsigned char *pEnd;
  char *ppSrcs[2];
  int lens[2];
  char szPad[3];
  int k;
  int loop;

  if (nSrcLen <= 0)
  {
       *dest = '\0';
       *dest_len = 0;
       return dest;
  }

  linePos = 0;
  lens[0] = (nSrcLen / 3) * 3;
  lens[1] = 3;
  leftover = nSrcLen - lens[0];
  ppSrcs[0] = src;
  ppSrcs[1] = szPad;

  szPad[0] = szPad[1] = szPad[2] = '\0';
  switch (leftover)
  {
      case 0:
      default:
           loop = 1;
           break;
      case 1:
           loop = 2;
           szPad[0] = src[nSrcLen-1];
           break;
      case 2:
           loop = 2;
           szPad[0] = src[nSrcLen-2];
           szPad[1] = src[nSrcLen-1];
           break;
  }

  pDest = dest;
  for (k=0; k<loop; k++)
  {
      pEnd = (unsigned char *)ppSrcs[k] + lens[k];
      for (pRaw=(unsigned char *)ppSrcs[k]; pRaw<pEnd; pRaw+=3)
      {
         // Start a new line if next 4 chars won't fit on the current line
         // We can't encapsulete the following code since the variable need to
         // be local to this incarnation of encode.
         linePos += 4;
         if (linePos > line_length)
         {
            if ( line_length != 0 )
            {
               memcpy(pDest, line_separator, line_sep_len);
               pDest += line_sep_len;
            }
            linePos = 4;
         }

         // get next three bytes in unsigned form lined up,
         // in big-endian order
         combined = ((*pRaw) << 16) | ((*(pRaw+1)) << 8) | (*(pRaw+2));

         // break those 24 bits into a 4 groups of 6 bits,
         // working LSB to MSB.
         c3 = combined & 0x3f;
         combined >>= 6;
         c2 = combined & 0x3f;
         combined >>= 6;
         c1 = combined & 0x3f;
         combined >>= 6;
         c0 = combined & 0x3f;

         // Translate into the equivalent alpha character
         // emitting them in big-endian order.
         *pDest++ = valueToChar[c0];
         *pDest++ = valueToChar[c1];
         *pDest++ = valueToChar[c2];
         *pDest++ = valueToChar[c3];
      }
  }

  *pDest = '\0';
  *dest_len = pDest - dest;

  // deal with leftover bytes
  switch (leftover)
  {
     case 0:
     default:
        // nothing to do
        break;
     case 1:
        // One leftover byte generates xx==
        if (bPad)
        {
           *(pDest-1) = pad_ch;
           *(pDest-2) = pad_ch;
        }
        else
        {
           *(pDest-2) = '\0';
           *dest_len -= 2;
        }
        break;
     case 2:
        // Two leftover bytes generates xxx=
        if (bPad)
        {
           *(pDest-1) = pad_ch;
        }
        else
        {
           *(pDest-1) = '\0';
           *dest_len -= 1;
        }
        break;
  } // end switch;

  return dest;
}