/******************************************************************************/
/* This module was designed explicitly for use with the AC-Delco price        */
/* reading tool.  These utilities are custom designed by Fritz Feuerbacher    */
/******************************************************************************/

#include <graphics.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <dos.h>
#include <sys\stat.h>
#include <io.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <process.h>
#include <stdarg.h>
#include <bios.h>

#include "ansidrv.h"
#include "colors.h"                  // vid mode & color definition header
#include "extra.h"                   // extra functions header
#include "fkeys.h"

#define  POP      0
#define  PUSH     1              /* used in screen save/restore function */


void beep()
{
  int i;

  for (i=1; i<40; i++)  {
       sound(i*5);
       delay(3);
    }
  nosound();
 }

void readstr( char *str, int maxindex, int init );
/***************************************************************************/
/* In:      str = character string for read from the keyboard              */
/*     maxindex = the total length of the string                           */
/*         init = boolean indicating whether or not to initialize str      */
/* Out: str = string read from the keyboard terminated by \0 (null)        */
/* This function allows backspacing.  It takes into account the string     */
/* size so you won't run into problems reading over other memory address'. */
/* To be continued. . . .  "The mother of all string editors"              */
/***************************************************************************/

/******************************************************************************/
void readstr( char *str, int maxindex, int init )  {

   int ch, i, x, xbegin, y;
   int extended = 0;
   if (init)  {
      for (i = 0; i <= maxindex-1; i++)
          str[i] = ' ';
      str[maxindex-1] = NULL;
    }
   i = 0; xbegin = wherex(); y = wherey();
   while ((i <= maxindex-1) && ((ch = getch()) != CR) && (ch != ESC))  {
          x = wherex();
          if (!ch)  {
             extended = getch();
             if (extended)  {
                switch (extended)  {

                  case LF   :
                              if (i > 0) {
                                 gotoxy(x-1, y);
                                 --i;
                               } /* if */
                              break;
                  case RT   :
                              if (i < maxindex-1) {
                                 gotoxy(x+1, y);
                                 ++i;
                               } /* if */
                              break;
                  case HOME :
                              if (i > 0) {
                                 gotoxy(xbegin, y);
                                 i = 0;
                               } /* if */
                              break;
                  case END  :
                              if (i < maxindex-1) {
                                 gotoxy(xbegin+maxindex-1, y);
                                 i = maxindex-1;
                               } /* if */
                              break;
                  case DEL  :
                            if (i < maxindex-1) {
                               putch(' ');
                               str[i++] = ' ';
                             } /* if */
                            break;
                   default   : ;
                  }  /* switch */
                } /* if */
             } /* if */
          else  {
            switch (ch)  {           /* a regular key was hit */

              case BS   :
                            if (i > 0) {
                               putch('\b');
                               putch(' ');
                               putch('\b');
                               str[--i] = ' ';
                             } /* if */
                            break;
             case HTAB  :
                            if (i < maxindex-6) {
                               gotoxy(x+5, y);
                               i += 5;
                             } /* if */
                            break;

              default   :
                           str[i++] = ch;
                           putch(ch);
              } /* switch */
          } /* else */
     } /* while */
 } /* readstr */

/******************************************************************************/
int read_fstr( char *str, int maxindex, int init )
{

   int ch, i, x, xbegin, y;
   int extended = 0;

    xbegin = wherex(); y = wherey();
    gotoxy( xbegin++, y ); putch('(');
    gotoxy( xbegin, y );
    if (init)  {
        for (i=0; i <=maxindex-1; i++)  {
           str[i] = ' ';
           putch('�');
        }
        str[maxindex-1] = NULL;
    }
   gotoxy( xbegin+maxindex, y ); putch(')');
   gotoxy( xbegin, y );

   i = 0;
   while ((i <= maxindex-1) && ((ch = getch()) != CR) && (ch != ESC))  {
          x = wherex();
          if (!ch)  {
             extended = getch();
             if (extended)  {
                switch (extended)  {

                  case LF   :
                              if (i > 0) {
                                 gotoxy(x-1, y);
                                 --i;
                               } /* if */
                              break;
                  case RT   :
                              if (i < maxindex-1) {
                                 gotoxy(x+1, y);
                                 ++i;
                               } /* if */
                              break;
                  case HOME :
                              if (i > 0) {
                                 gotoxy(xbegin, y);
                                 i = 0;
                               } /* if */
                              break;
                  case END  :
                              if (i < maxindex-1) {
                                 gotoxy(xbegin+maxindex-1, y);
                                 i = maxindex-1;
                               } /* if */
                              break;
                  case DEL  :
                            if (i < maxindex-1) {
                               putch(' ');
                               str[i++] = ' ';
                             } /* if */
                            break;
                   default   : ;
                  }  /* switch */
                } /* if */
             } /* if */
          else  {
            switch (ch)  {           /* a regular key was hit */

              case BS   :
                            if (i > 0) {
                               putch('\b');
                               putch(' ');
                               putch('\b');
                               str[--i] = ' ';
                             } /* if */
                            break;
             case HTAB  :
                            if (i < maxindex-6) {
                               gotoxy(x+5, y);
                               i += 5;
                             } /* if */
                            break;

              default   :
                           str[i++] = ch;
                           putch(ch);
              } /* switch */
          } /* else */
     } /* while */
     if (ch == ESC)
        return 0;
     else
        return 1;
 } /* readstr */

/******************************************************************************/
int read_part( char *str, int maxindex, int init )
/******************************************************************************/
/* This is a highly specialized function.  It does a lot!                     */
/******************************************************************************/

{

   int ch, i, x, xbegin, y;
   int extended = 0;

    xbegin = wherex(); y = wherey();
    gotoxy( xbegin++, y ); putch('(');
    gotoxy( xbegin, y );
    if (init)  {
        for (i=0; i<=maxindex-1; i++)
            str[i] = ' ';
        str[maxindex-1] = NULL;
    }
   gotoxy( xbegin+maxindex, y ); putch(')');
   gotoxy( xbegin, y );

   i = 0;
   while ((i <= maxindex-1) && ((ch = getch()) != CR) && (ch != ESC))  {
          x = wherex();
          if (!ch)  {
             extended = getch();
             if (extended)  {
                switch (extended)  {

                  case LF   :
                              if (i > 0) {
                                 gotoxy(x-1, y);
                                 --i;
                               } /* if */
                              break;
                  case RT   :
                              if (i < maxindex-1) {
                                 gotoxy(x+1, y);
                                 ++i;
                               } /* if */
                              break;
                  case HOME :
                              if (i > 0) {
                                 gotoxy(xbegin, y);
                                 i = 0;
                               } /* if */
                              break;
                  case END  :
                              if (i < maxindex-1) {
                                 gotoxy(xbegin+maxindex-1, y);
                                 i = maxindex-1;
                               } /* if */
                              break;
                  case DEL  :
                            if (i < maxindex-1) {
                               putch(' ');
                               str[i++] = ' ';
                             } /* if */
                            break;
                  case F1   : return F1;
                  case F2   : return F2;
                  case F3   : return F3;
                  case F4   : return F4;
                  case F5   : return F5;
                  case F6   : return F6;
                  case F7   : return F7;
                  case F8   : return F8;
                  case F9   : return F9;
                  case PGUP : return PGUP;
                  case PGDN : return PGDN;

                   default  : ;
                  }  /* switch */
                } /* if */
             } /* if */
          else  {
            switch (ch)  {           /* a regular key was hit */

              case BS   :
                            if (i > 0) {
                               putch('\b');
                               putch(' ');
                               putch('\b');
                               str[--i] = ' ';
                             } /* if */
                            break;
             case HTAB  :
                            str[i++] = '-';
                            putch('-');
                            break;            /* both htab and space */
              case SPC  :                     /* produce dashes      */
                            str[i++] = '-';
                            putch('-');
                            break;
              default   :
                           str[i++] = toupper(ch);
                           putch(toupper(ch));
              } /* switch */
          } /* else */
     } /* while */
   if (ch == ESC)
      return ESC;
   if ((ch==CR) && (i==0))
      return PGDN;
   else
      return CR;
 } /* read_part */

void read_line(char *str, int maxindex)
/*****************************************************/
/* Read product line only.  Str must be 2 chars long */
/*****************************************************/
{
  char ch;
  int  i;

   for (i=0; i<=maxindex-1; i++)
         str[i] = ' ';
   str[maxindex-1] = NULL;
   i = 0;

   while ((i <= maxindex-1) && ((ch = getch()) != CR) && (ch != ESC))  {
       if (!ch)  {                /* extended key was hit  */
          getch();
          beep();
       }
       else  {
          switch (ch)  {           /* a regular key was hit */
              case BS   :
                            if (i > 0) {
                               putch('\b');
                               putch(' ');
                               putch('\b');
                               str[--i] = ' ';
                             } /* if */
                            break;
       case SPC, HTAB   :   beep();
                            break;
   
              default   :
                           str[i++] = toupper(ch);
                           putch(toupper(ch));
          }
       }
   }

  if (i < maxindex-1)
      str[i] = NULL;
}

void convert_price( char *str );
/***************************************************************************/
/* In:      str = AC-Delco price character string from price file.         */
/* Out:     str = string converted to look like money.                     */
/***************************************************************************/

/******************************************************************************/
void convert_price( char *str )
{
  int i;

  if (strlen(str) > 6)
     str[strlen(str)-1] = ' ';

  for (i=0; i<=(strlen(str)-1); i++)  {
     if (str[i] == '0')
        str[i] = ' ';
     else
        break;
  }
}

/******************************************************************************/
int acd_logo()
{
  int gdriver = DETECT,  gmode;
  int errorcode;
  char string[] = "ACDelco";
  int i = 0;

  initgraph(&gdriver, &gmode, "");
  errorcode = graphresult();

  if (errorcode == grOk)   {    /* graphics error did not occur */

    setfillstyle(SOLID_FILL, BLACK);
	settextjustify(CENTER_TEXT,CENTER_TEXT);
    setusercharsize(100,100,100,100);
    settextstyle(SANS_SERIF_FONT,HORIZ_DIR,USER_CHAR_SIZE);
    outtextxy(getmaxx()/2,getmaxy()/3, string);

    for (i=0; i<4; i++)  {

       delay(15);
       bar((getmaxx()/2)-(textwidth(string)/2),(getmaxy()/3)-(textheight(string)*3/2),
          (getmaxx()/2)+(textwidth(string)/2),(getmaxy()/3)+textheight(string)*3/2);
       setusercharsize(2+i,1,2+i,1);
       settextstyle(SANS_SERIF_FONT,HORIZ_DIR,USER_CHAR_SIZE);
       outtextxy(getmaxx()/2, getmaxy()/3, string);

    }
    moveto(20,250);
    lineto(580,250);
    lineto(10,270);
    lineto(20,250);
    setusercharsize(150,150,150,150);
    settextstyle(SANS_SERIF_FONT,HORIZ_DIR,USER_CHAR_SIZE);
    outtextxy(getmaxx()/2, getmaxy()/1.5, "Powers The Winners");

    setusercharsize(1,2,1,2);
    settextstyle(SANS_SERIF_FONT,HORIZ_DIR,USER_CHAR_SIZE);
    outtextxy(getmaxx()/2, getmaxy()/1.1, "Press any key");
    getch();
	closegraph();
	return(0);
  }
  return 1;
}
/******************************************************************************/
int get_init(char *price_dir, char *cross_dir, char *comp_name)
{
  FILE *fp;
  int  i;

        if ((fp = fopen( "ACDPRICE.INI", "r")) == NULL)
             return 1;
        else  {
           for (i=0; i<=59; i++)
               price_dir[i] = fgetc(fp);
           for (i=0; i<=59; i++)
               cross_dir[i] = fgetc(fp);
           for (i=0; i<39; i++)
               comp_name[i] = fgetc(fp);

           fclose( fp );
           return 0;
        }
}

void init_string( char *str, int len )
{
  int i;

  for (i=0; i <= len-1; i++)
      str[i] = ' ';

  str[len-1] = NULL;
}
//************************************************************************
void display_triad() // Display a Triad type screen
{

d_msgat(0, 0, RVRS, "Line�Part Number  � Description �GM Number � Pop Code �History �Factory Item    ");
d_msgat(4, 0, RVRS, "Special App �  Supered Stat �Supered Part#  �  Unit Weight  � Package Weight    ");
d_msgat(8, 0, RVRS, "  Jobber Price   �     Dealer Price      �    Trade Price    �   List Price     ");
d_msgat(12, 0, RVRS," Resale Core �   Truck Price  �  Less Truck  �  Retail Price   �  Carton Qty    ");
d_msgat(16, 0, RVRS,"Subline�Pallet Qty �   Pallet UPC   �    Merchandise UPC    �     Carton UPC    ");

}

void display_lines()
{
SETWND(0, 0, 24, 80);
cls();
printf("                             AC-Delco Product Lines\n");
printf("                         ������������������������������\n\n");
printf("           01) � Electric / Ignition          29) � Oil Seal\n");
printf("           02) � Bearing                      31) � Starter / Alternator\n");
printf("           03) � Speedometer / Dashboard      36) � Front Wheel Drive\n");
printf("           04) � Radiator / Heater Core       37) � Transmission\n");
printf("           05) � Shock / Strut                41) � Spark Plug\n");
printf("           06) � Reman Radio / Cluster        42) � Oil Filter / Element\n");
printf("           07) - Battery                      43) � Fuel Pump\n");
printf("           09) � Carburetor / Fuel Inj.       44) � Cable & Casing\n");
printf("           10) � Chemical                     47) � Gas Filter\n");
printf("           13) � Thermostat                   48) � Air Filter\n");
printf("           15) � Air Conditioning             49) � Gauge\n");
printf("           16) � Wire and Cable               50) � Transmission Filter\n");
printf("           17) � Brake Product                51) � Valve Lifter / Rem Engine\n");
printf("           18) � Automotive Motor             53) � Breather / Gas-Oil-Rad Cap\n");
printf("           21) � Emission Control             56) � Headlamp / Mini-Lamp\n");
printf("           22) � Engine Head / Oil Pump       57) � PCV Valve\n");
printf("           25) � Water Pump                   59) � Cable & Casing Part\n");
printf("           27) � Antenna / Radio              60) � Misc. Tool\n");
printf("                                              61) � Oil Filter Wrench\n");
printf("\n\n%s", "                               Press any key");
getch();

}
