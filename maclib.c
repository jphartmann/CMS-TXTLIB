/* Build EBCDIC MACLIB from ASCII COPY/MACRO files                   */
/*                                John Hartmann 18 Jun 2016 10:28:34 */

/*********************************************************************/
/* Input is COPY and MACRO files.                                    */
/*                                                                   */
/* For COPY, if the first record begins *COPY, which means a stacked */
/* file,   we  take  member  names  from  these  delimiter  records. */
/* Otherwise the member name is the uppercased file name and we have */
/* one member for each input file.                                   */
/*                                                                   */
/* Change activity:                                                  */
/*18 Jun 2016  New module.                                           */
/*********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "libpds.h"

static unsigned char sep[80] = {0x61, 0xff, 0xff, 0x61};
static const int cards = 100;
static FILE * f;
static char line[256];

const unsigned char tab819to1047[256]={
/*                                     6           8                 */
   0x00, 0x01, 0x02, 0x03, 0x37, 0x2D, 0x2E, 0x2F, 0x16, 0x05, 0x25, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x3C, 0x3D, 0x32, 0x26, 0x18, 0x19, 0x3F, 0x27, 0x1C, 0x1D, 0x1E, 0x1F,
   0x40, 0x5A, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D, 0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,
   0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F,
   0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
   0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xAD, 0xE0, 0xBD, 0x5F, 0x6D,
   0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,
   0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xC0, 0x4F, 0xD0, 0xA1, 0x07,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x15, 0x06, 0x17, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x09, 0x0A, 0x1B,  /* 8 */
   0x30, 0x31, 0x1A, 0x33, 0x34, 0x35, 0x36, 0x08, 0x38, 0x39, 0x3A, 0x3B, 0x04, 0x14, 0x3E, 0xFF,
   0x41, 0xAA, 0x4A, 0xB1, 0x9F, 0xB2, 0x6A, 0xB5, 0xBB, 0xB4, 0x9A, 0x8A, 0xB0, 0xCA, 0xAF, 0xBC,
   0x90, 0x8F, 0xEA, 0xFA, 0xBE, 0xA0, 0xB6, 0xB3, 0x9D, 0xDA, 0x9B, 0x8B, 0xB7, 0xB8, 0xB9, 0xAB,
   0x64, 0x65, 0x62, 0x66, 0x63, 0x67, 0x9E, 0x68, 0x74, 0x71, 0x72, 0x73, 0x78, 0x75, 0x76, 0x77,  /* c */
   0xAC, 0x69, 0xED, 0xEE, 0xEB, 0xEF, 0xEC, 0xBF, 0x80, 0xFD, 0xFE, 0xFB, 0xFC, 0xBA, 0xAE, 0x59,
   0x44, 0x45, 0x42, 0x46, 0x43, 0x47, 0x9C, 0x48, 0x54, 0x51, 0x52, 0x53, 0x58, 0x55, 0x56, 0x57,
   0x8C, 0x49, 0xCD, 0xCE, 0xCB, 0xCF, 0xCC, 0xE1, 0x70, 0xDD, 0xDE, 0xDB, 0xDC, 0x8D, 0x8E, 0xDF,
};

/* Forward declarations:                                             */
static int doline(const char * s);
static int loadname(const char * name, int len);
static int newmember(char * s);
static int eom();
static char * gline(void);
/* End of forward declarations.                                      */

#include "subs.c"

/*********************************************************************/
/* Main loop.                                                        */
/*********************************************************************/

int
main(int argc, char ** argv)
{
   char * fn;
   int ix;

   if (3 > argc)
      return fprintf(stderr, "Specify library file name and copy file name(s).\n"), 1;
   if (openout(argv[1])) return 16;

   if (getdir(cards * 80)) return 20;

   for (ix = 2; argv[ix]; ix++)
   {
      char * dot;
      char * slash;
      char * name;
      int namelen;
      int ismacro = 1;                /* Assume file is macro        */
      char * s;

      fn = argv[ix];
      f = fopen(fn, "r");
      if (!f) return fprintf(stderr, "Cannot open %s:  %s\n", fn, strerror(errno)), 16;

      slash = strrchr(fn, '/');
      name = slash ? slash + 1 : fn;
      namelen = strlen(name);

      dot = strrchr(name, '.');
      if (dot)
      {
         namelen = dot - name;
         if (!strcmp(dot + 1, "copy")) ismacro = 0;
      }

      s = gline();
      if (!s) continue;
      if (!ismacro && memcmp(s, "*COPY ", 6)) ismacro = 1;   /* Not stacked copy */
      if (ismacro)
      {
         loadname(name, namelen);
         if (doline(line)) return 20;
      }
      else if (newmember(s)) return 20;
      for (;;)
      {
         s = gline();
         if (!s) break;
         if (!ismacro && !memcmp(s, "*COPY ", 6))
         {
            if (eom()) return 20;
            if (newmember(s)) return 20;
            continue;
         }
         if (doline(line)) return 20;
      }
      if (eom()) return 20;
   }
   if (wrapup(dirs, dir)) return 16;
   printf("%s %d files %d members.\n",
      fno, ix - 2, members);
   return 0;
}

/*********************************************************************/
/* Read a line and remove the newline.                               */
/*********************************************************************/

static char *
gline(void)
{
   char * s = fgets(line, sizeof(line), f);
   int len;

   if (!s) return s;                  /* Null file                   */
   len = strlen(s);
   if ('\n' == s[len - 1]) s[len - 1] = 0;
   return s;
}

/*********************************************************************/
/* Get member name from *COPY record.                                */
/*********************************************************************/

static int
newmember(char * s)
{
   char * t;

   s += 5;
   s += strspn(s, " ");
   t = strpbrk(s, " ");

   return loadname(s, t ? t - s : strlen(s));
}

/*********************************************************************/
/* Create a directory entry in the current directory block.          */
/*********************************************************************/

static int
loadname(const char * name, int len)
{
   struct dirent * de;
   int i;

   if (dir->count * 5 <= dir->members && getdir(cards)) return 1;

   if (8 < len)
   {
      fprintf(stderr, "Member name %.*s truncated to 8 characters.\n",
         len, name);
      len = 8;
   }
   members++;                         /* in all buffers              */
   de = dir->dents + dir->members++;
   memset(de->name, 0x40, 8);
   for (i = 0; len > i; i++)
      de->name[i] = tab819to1047[toupper(0xff & name[i])];
   memset(de->fill, 0, 4);
   BEINT(de->beitem, dirptr);
   return 0;
}

/*********************************************************************/
/* Pad, translate, and write a line.                                 */
/*********************************************************************/

static int
doline(const char * s)
{
   char line[80];
   int i;

   memset(line, 0x40, 80);
   for (i = 0; 80 > i && s[i]; i++)
      line[i] = tab819to1047[0xff & s[i]];
   return dump(line);
}

/*********************************************************************/
/* Write end-of-member record                                        */
/*********************************************************************/

static int
eom()
{
   if (dump(sep)) return 1;
   if (0xff & dirptr) return 0;       /* TTR not tt0                 */
   return dump(sep);
}
