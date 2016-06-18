/* Create txtlib from stacked decks (CMS TXTLIB cannot do this).     */
/*                                John Hartmann 17 Jun 2016 11:23:10 */

/*********************************************************************/
/* Input are F(80) TEXT files, already EBCDIC.                       */
/*                                                                   */
/* Output  is  a  TXTLIB  with directory entries for each SD and LD. */
/* Unlike  CMS  we do not add the file name as a directory entry; we */
/* could do this for the special case of a single ESD/END in a file. */
/*                                                                   */
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
#include "libpds.h"

static int fdin;

static unsigned char ldt[84] = {0x61, 0xff, 0xff, 0x61, 2, 0xd3, 0xc4, 0xe3,};
/* item is zeroed by END card; flag for make new member           */
static struct dirent de = {.flag = 0x80, .beitem[3] = 2, };
static struct file * currfile;
static struct file * currdir;
static int pads;

/* Forward declarations:                                             */
static void doesd(struct card * crd);
static int doend(struct card * crd);
static void loaddir(void);
/* End of forward declarations.                                      */

#include "subs.c"

/*********************************************************************/
/* Main routine.  check args &c.                                     */
/*********************************************************************/

int
main(int argc, char ** argv)
{
   struct stat st;
   int rv;
   size_t done;
   char * fn;
   int cards;
   int fsize;
   int ix;

   if (3 > argc)
      return fprintf(stderr, "Specify library file name and text file name(s).\n"), 1;
   if (openout(argv[1])) return 16;

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
      if (getdir(st.st_size)) return 20;
      currfile = dir;

      if (2 == ix)
      {
         currdir = currfile;          /* First time                  */
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

   if (wrapup(dirs, currdir)) return 20;

   fstat(fdout, &st);
   close(fdout);
   printf("%s %d files %d members %ld card%s %d pads.\n",
      fno, ix - 2, members, (long) st.st_size / 80, (2 < dirptr ? "s" : ""), pads);
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
      {
         if (!currdir->next)
         {
            printf("Directory entries gone berserk.  Count %d members %d.\n",
               currdir->count, currdir->members);
            fflush(stdout);
         }
         else
         {
            printf("Switching dir.  %d cards %d members full.  %d cards %d members in next.\n",
               currdir->count, currdir->members,
               currdir->next->count, currdir->next->members);
            fflush(stdout);
         }
         currdir = currdir->next;
      }
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
