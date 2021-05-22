#include <stdio.h>
#include <io.h>
#include <alloc.h>
#include <fcntl.h>
#include <process.h>
#include <sys\stat.h>
#include <dos.h>
#include <conio.h>

void main()
{
  struct stat statbuf;
  char string[10];
  int length, res;
  char ch;

  fstat( fileno(stdprn), &statbuf );
   /* display the information returned */
   if (statbuf.st_mode & S_IFCHR)
      printf("Handle refers to a device.\n");
   if (statbuf.st_mode & S_IFREG)
      printf("Handle refers to an ordinary file.\n");
   if (statbuf.st_mode & S_IREAD)
      printf("User has read permission on file.\n");
   if (statbuf.st_mode & S_IWRITE)
      printf("User has write permission on file.\n");

   printf("Drive letter of file: %c\n", 'A'+statbuf.st_dev);
   printf("Size of file in bytes: %ld\n", statbuf.st_size);
   printf("Time file last opened: %s\n", ctime(&statbuf.st_ctime));

   /* force an error condition by attempting to read */
   fputc(ch, stdprn);

   fflush(stdprn);

   if (ferror(stdprn))
   {
      /* display an error message */
      printf("Error reading from stdprn\n");

      /* reset the error and EOF indicators */
      clearerr(stdprn);
   }

/*
   if ((res = write(fileno(stdprn), string, 1)) != 1)
   {
      printf("Error writing to the file.\n");
      exit(1);
   }
   printf("Wrote %d bytes to the file.\n", res);
 */

}

