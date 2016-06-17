/* Create txtlib from stacked decks (CMS TXTLIB cannot do this).     */
/*                                John Hartmann 17 Jun 2016 11:23:10 */

/*********************************************************************/
/*

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
*/
/* Change activity:                                                  */
/*17 Jun 2016  New module.                                           */
/*********************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "objdeck.h"

#define BEINT(c, v) (c[0] = 0xff & ((v) >> 24)), (c[1] = 0xff & ((v) >> 16)), \
   (c[2] = 0xff & ((v) >>  8)), (c[3] = 0xff & (v))

struct pdslib
{
   unsigned char ident[6];
   char fill[2];
   unsigned char bemembers[4];
   unsigned char bedirptr[4];
   unsigned char fill2[80 - 16];
};

static int members;
static int dirptr = 1;

struct dirent
{
   unsigned char name[8];
   unsigned char fill[3];
   unsigned char flag;                /* x'80', new format           */
   unsigned char beitem[4];                      /* Record number               */
};

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

static unsigned char ldt[84] = {0x61, 0xff, 0xff, 0x61, 2, 0xd3, 0xc4, 0xe3,};
static char * fno;
static int fdin, fdout;
static struct pdslib hdr =
{
   .ident = {0xd3, 0xc9, 0xc2, 0xd7, 0xc4, 0xe2, },
};
/* item is zeroed by END card; flag for make new member           */
static struct dirent de = {.flag = 0x80, .beitem[3] = 2, };
static struct file * files;
static struct file * currfile;
static struct file * currdir;
static struct file ** pfiles = &files;
static int pads;

/* Forward declarations:                                             */
static int dump(void * data);
static void doesd(struct card * crd);
static int doend(struct card * crd);
static void loaddir(void);
/* End of forward declarations.                                      */

int
main(int argc, char ** argv)
{
   struct stat st;
   int rv;
   size_t done;
   char * fn = argv[1];
   int cards;
   int fsize;
   int fill;
   int ix;

   if (3 > argc) return fprintf(stderr, "Specify text file name and library file name.\n"), 1;
   fno = argv[1];
   fdout = open(fno, O_CREAT | O_WRONLY | O_TRUNC, 0666);
   if (-1 == fdout) return fprintf(stderr, "Cannot open %s:  %s\n", fn, strerror(errno)), 16;

   for (ix = 2; argv[ix]; ix++)
   {
      fn = argv[ix];
      fdin = open(fn, O_RDONLY);
      if (-1 == fdin) return fprintf(stderr, "Cannot open %s:  %s\n", fn, strerror(errno)), 16;

      rv = fstat(fdin, &st);
      if (-1 == rv) return fprintf(stderr, "Cannot stat %s:  %s\n", fn, strerror(errno)), 16;

      if (!S_ISREG(st.st_mode))
         return fprintf(stderr, "Not a regular file: %s; mode is %x\n", fn, st.st_mode), 12;

      fsize = st.st_size / 80;
      currfile = malloc(st.st_size + sizeof(struct file));
      if (!currfile)
         return fprintf(stderr, "Cannot allocate %ld bytes for %s\n", (long) st.st_size, fn), 20;

      memset(currfile, 0, sizeof(struct file));
      *pfiles = currfile;
      pfiles = &currfile->next;
      currfile->count = fsize;
      if (2 == ix)
      {
         currdir = currfile;          /* First time                  */
         if (dump(&hdr)) return 12;   /* Reserve space in file       */
      }

      for (done = 0; st.st_size > done;)
      {
         ssize_t got = read(fdin, currfile->cards + done, st.st_size - done);

         if (-1 == got) return fprintf(stderr, "Cannot read %s:  %s\n", fn, strerror(errno)), 16;
         if (!got) return fprintf(stderr, "Premature EOF on %s:  %d cards read.\n", fn, done / 80), 16;
         done += got;
      }

      close(fdin);


      /******************************************************************/
      /* Dump  records  to  output  file.   For  SD  and LD ESDs, add a */
      /* directory  entry.   The  directory  can overlay the file if we */
      /* write the ESD record before inspecting it.                     */
      /******************************************************************/

      for (cards = 0; fsize > cards; cards++)
      {
         struct card * crd = currfile->cards + cards;

         if (dump(crd)) return 12;
         if (2 != crd->ctype[0]) continue;
         if (!memcmp(crd->ctype + 1, E_ESD, 3)) doesd(crd);
         else if (!memcmp(crd->ctype + 1, E_END, 3) && doend(crd)) return 16;
      }
   }

   /******************************************************************/
   /* Pad  out  last  directory record, load bigendian in header and */
   /* wrap up.                                                       */
   /******************************************************************/

   BEINT(hdr.bemembers, members);
   BEINT(hdr.bedirptr, dirptr);


   fill = members % 5;                /* Partial record              */
   if (fill)
   {
      memset(&de, 0, sizeof(de));
      while (5 > fill++) loaddir();
   }

   for (currdir = files; currdir && currdir->members; currdir = currdir->next)
   {
      fill = ((currdir->members + 4) / 5) * 80;    /* Directory size */
      write(fdout, currdir->dents, fill);
      dirptr += currdir->members / 5;
   }

   lseek(fdout, 0, SEEK_SET);            /* Back to beginning;          */
   write(fdout, &hdr, 16);

   fstat(fdout, &st);
   close(fdout);
   printf("%s %d files %d members %d card%s %d pads.\n",
      fno, ix - 2, members, dirptr - 1, (2 < dirptr ? "s" : ""), pads);
   return 0;
}

static int
dump(void * data)
{
   ssize_t rv = write(fdout, data, 80);

   if (80 != rv)
      return fprintf(stderr, "Cannot write 80 bytes to %s:  %s.\n",
         fno, (-1 == rv ? strerror(errno) : "<incomplete write>"));

   dirptr++;
   return 0;
}

/*********************************************************************/
/* Process ESD records to extract SD and LD names.                   */
/*********************************************************************/

static void
doesd(struct card * crd)
{
   int i;
   const int size = (crd->vfl[0] << 8) | crd->vfl[1];
   const int cnt = size / sizeof(struct esditem);

   for (i = 0; cnt > i; i++)
   {
      struct esditem * ei = crd->d.esd + i;

      if (1 < ei[i].type) continue;            /* Not SD or LD */
      memcpy(de.name, ei[i].symbol, 8);
      if (currdir->count * 5 <= currdir->members)
         currdir = currdir->next;
      members++;
      loaddir();
   }
}

static void
loaddir(void)
{
   currdir->dents[currdir->members++] = de;
}

/*********************************************************************/
/* Process END card to generate two loader terminate records.  Pad a */
/* record  if  the  record number is divisible by 256 to avoid a TTR */
/* with R 0.                                                         */
/*********************************************************************/

static int
doend(struct card * crd)
{
   if (dump(ldt + 4)) return 16;
   if (dump(ldt)) return 16;
   if (!(0xff & dirptr))
   {
      if(dump(ldt)) return 16;        /* Placeholder for R0          */
      pads++;
   }
   BEINT(de.beitem, dirptr);
   return 0;
}
