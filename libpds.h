/* CMS PDS format                                                    */
/*                                John Hartmann 18 Jun 2016 10:29:53 */

/*********************************************************************/
/* Change activity:                                                  */
/*18 Jun 2016  New header file.                                      */
/*********************************************************************/


#if !defined(_JPH_LIBPDS_H)
#define _JPH_LIBPDS_H

#if 0
PDSIDENT DS    C'LIBPDS'          MACLIB/PDS IDENTIFIER.
PDSFNEW  EQU   C'P'               CHECK PDSIDENT+3, OLD VS NEW.
PDSFLG1  DS    X                  MACLIB/PDS FLAG1.
PDSTEMPF EQU   C'$'               PDS DIR. IS IN $PDSTEMP FILE.
PDSFLG2  DS    X                  MACLIB/PDS FLAG2.
PDSDIRSZ DS    F                  MACLIB/PDS DIRECTORY SIZE.
PDSDIRIT DS    F                  MACLIB/PDS DIRECTORY ITEM NO.
PDSHDRSZ EQU   *-PDSIDENT         SIZE OF MACLIB/PDS HEADER.
PDSENTSZ DS    F                  PDS ENTRY SIZE.

LIBPDS     -
DCCDCE00000600020002000044444444
39274200000000290000000200000000
 ESD            $TSEARCH       q
0CEC4444440344005EECCDCC00000019
2524000000000001B32519380000E098
 ESD            TDELETE    U
0CEC444444034444ECCDCEC4000E4000
25240000000000003453535010540001
...
 END
0CDC4444444444444444444444444444
25540000000000000000000000000000
 LDT
0DCE4444444444444444444444444444
23430000000000000000000000000000
/  / LDT
6FF60DCE444444444444444444444444
1FF12343000000000000000000000000
$TSEARCH        TDELETE
5EECCDCC00080000ECCDCEC400080000
B3251938000000023453535000000002
#endif

#include "objdeck.h"

#define BEINT(c, v) (c[0] = 0xff & ((v) >> 24)), (c[1] = 0xff & ((v) >> 16)), \
   (c[2] = 0xff & ((v) >>  8)), (c[3] = 0xff & (v))

/*********************************************************************/
/* Header record ina library                                         */
/*********************************************************************/

struct pdslib
{
   unsigned char ident[6];
   char fill[2];
   unsigned char bemembers[4];
   unsigned char bedirptr[4];
   unsigned char fill2[80 - 16];
};

/*********************************************************************/
/* Directory entry.                                                  */
/*********************************************************************/

struct dirent
{
   unsigned char name[8];
   unsigned char fill[3];
   unsigned char flag;                /* x'80', new format           */
   unsigned char beitem[4];           /* Record number               */
};

/*********************************************************************/
/* A file or a buffer for directory entries or both.                 */
/*********************************************************************/

struct file
{
   struct file * next;
   int count;                        /* Number of cards in the file; */
   int members;                       /* Active member entries       */
   union
   {
      struct card cards[0];
      struct dirent dents[0];
   };
};

#endif
