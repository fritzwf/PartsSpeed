#include <stdio.h>
#include <sys\stat.h>
#include <io.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <string.h>
#include <process.h>
#include <stdarg.h>
#include <dos.h>
#include <bios.h>

char localnum[19], logonid[19], initstring[49];
char com_str[5];
char params[10];

void main()
{
    FILE *logfil;
    int ch;
    unsigned  i;
    if (NULL != (logfil = fopen("termparm.ini", "r")))  {
   localnum[0] = logonid[0] = initstring[0] = com_str[0] = params[0] = '\0';
        for (i=0; i<=12; i++)  {
        ch = fgetc(logfil);
        ch = ch >> 1;
        logonid[i] = ch;
        }
        for (i=0; i<=13; i++) localnum[i] = (fgetc(logfil));
        for (i=0; i<=43; i++) initstring[i] = (fgetc(logfil));
        for (i=0; i<=3; i++)  com_str[i] = (fgetc(logfil));
        for (i=0; i<=6; i++)  params[i] = (fgetc(logfil));
        fclose(logfil);
        printf("\n\nUserid/password: %s", logonid );
        printf("\nPartSpeed phone: %s", localnum );
        printf("\n     Modem init: %s", initstring );
        printf("\n       COM port: %s", com_str );
        printf("\n COM parameters: %s\n", params );
      }
    else  {
      printf( "%s\n", "File termparm.ini does not exist!");
      exit(0);
    }
}
