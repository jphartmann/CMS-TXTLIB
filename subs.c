/* Common code between maclib and txtlib                             */
/*                                John Hartmann 18 Jun 2016 12:00:25 */

/*********************************************************************/
/* Change activity:                                                  */
/*18 Jun 2016  New module.                                           */
/*********************************************************************/

static int dirptr = 1;
static int members;
static char * fno;
static int fdout;
static struct pdslib hdr =
{
   .ident = {0xd3, 0xc9, 0xc2, 0xd7, 0xc4, 0xe2, },
};
static struct file * dir;
static struct file * dirs;
static struct file ** pdir = &dirs;

/* Forward declarations:                                             */
/* End of forward declarations.                                      */

/*********************************************************************/
/* Write a card to the output.                                       */
/*********************************************************************/

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
/* Open the output file and set its name                             */
/*********************************************************************/

static int
openout(char * fname)
{
   fno = fname;
   fdout = open(fno, O_CREAT | O_WRONLY | O_TRUNC, 0666);
   if (-1 == fdout) return fprintf(stderr, "Cannot open %s:  %s\n", fno, strerror(errno));
   if (dump(&hdr)) return 12;   /* Reserve space in file       */
   return 0;
}

/*********************************************************************/
/* Pad  out last directory record, load bigendian in header and wrap */
/* up.                                                               */
/*********************************************************************/

static int
wrapup(struct file * firstfile, struct file * cur)
{
   struct file * fl;
   int fill = cur->members % 5;       /* Partial record              */

   BEINT(hdr.bemembers, members * 16);            /* Size, not count */
   BEINT(hdr.bedirptr, dirptr);

   if (fill)
   {
      struct dirent * de = cur->dents + cur->members;

      memset(de, 0, (5 - fill) * 16);
   }

   for (fl = firstfile; fl && fl->members; fl = fl->next)
   {
      const int cards = ((fl->members + 4) / 5) * 80; /* Directory size */

      write(fdout, fl->dents, cards);
   }

   lseek(fdout, 0, SEEK_SET);            /* Back to beginning;          */
   write(fdout, &hdr, 16);
   return 0;
}

/*********************************************************************/
/* Allocate a directory buffer.                                      */
/*********************************************************************/

static int
getdir(size_t size)
{
   const int toget = size + sizeof(struct file);

   if (toget) dir = malloc(toget);
   else
   {
      printf("Not getting any buffer:  %d\n", size);
      fflush(stdout);
      dir = 0;
   }

   if (!dir)
      return fprintf(stderr, "Cannot allocate %d bytes for file/directory buffer.\n", toget);
   memset(dir, 0, sizeof(struct file));
   dir->count = size / 80;
   *pdir = dir;
   pdir = &dir->next;
   return 0;

   printf("Allocated %d cards size %d at %p\n", dir->count, size, dir);
   fflush(stdout);
   return 0;
}
