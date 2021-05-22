#include <stdio.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <string.h>
#include <process.h>
#include <stdarg.h>
#include <time.h>
#include <dos.h>
#include <bios.h>

#include "comm.h"                            /* header for async functions */
#include "ansidrv.h"                   /* header for ANSI display routines */
#define KEY_INT
#include "keys.h"                                 /* key definition header */
#if defined(RED)
    #undef RED
#endif
#include "colors.h"                  /* vid mode & color definition header */
#include "extra.h"                               /* extra functions header */

#define   NO_FILES(x)     findfirst((x), &findbuf, 0)
#define   find_t          ffblk
#define   KBHIT           bioskey(1)
#define   KBREAD          bioskey(0)
#include  <dir.h>

#define POP 		0
#define PUSH		1			   /* used in screen save/restore function */
#define RXBUFSZ 	4096							/* rx ring buffer size */
#define TXBUFSZ 	1050							/* tx ring buffer size */
#define LOST_CARRIER -1
#define NO_INPUT	 -1
#define ESC_PRESSED  -2
#define TIMED_OUT	 -3

#define REG 	register

typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int uint;


/*		f u n c t i o n   d e c l a r a t i o n s	  */
static int proc_rxch(int ch);
static int watch_cd(void );
static int rcls(void);
static int toggle_echo(void );
static int toggle_dtr(void );
static void exit_term(void );
static int async_tx_str(char *str);
static int async_tx_echo(char *str);
static int rx_timeout(int Tenths);
static int waitfor(char *str, int ticks);
static int waitforOK(int ticks );
static int hang_up(void );

/*		g l o b a l   v a r i a b l e s 	*/

ASYNC	*port;								 /* pointer to async structure */
char	com_str[5]; 				   /* text for port ("COM1" or "COM2") */
int 	IOadrs = 0x3f8; 					 /* comm port base I/O address */
char	irqm = IRQ4;						 /* mask for selected IRQ line */
char	vctrn = VCTR4;						  /* comm interrupt vector nbr */

FILE	*logfil = NULL; 								/* log file handle */
char	dialstr[40];						   /* dial string storage area */
int 	ChkgKbd = 1;			  /* watch for ESC during rcv with timeout */
int 	ChkgCarrier = 0;		/* monitor carrier during rcv with timeout */
char	buf[200];									 /* gen purpose buffer */
int 	lfs = 1, echo = 0, dtr = 1; 		  /* various flags */
struct find_t findbuf;						  /* struct for NO_FILES macro */
int 	cyn, hred, grn, hgrn, wht, hblu, rev;	   /* text color variables */
char localnum[19], logonid[19], initstring[49];
char  day;
int news;
time_t tt;
struct tm *ptr;
/*
			* * * *    M A I N	  * * * *
*/
void main()
{
	static char params[10] = "2400E71";              /* default parameters */
	char		buf[100];
    int         chr, i, j=0;
    unsigned    ch;

	if (NULL != (logfil = fopen("newsparm.dat", "r")))  {
	localnum[0] = logonid[0] = initstring[0] = '\0';
		for (i=0; i<=12; i++)  {
           chr = fgetc(logfil);
           chr = chr >> 1;
           if (chr != ' ')
              logonid[j++] = chr;
             else
               continue;
		 }
        logonid[j] = '\0';
		for (i=0; i<=13; i++) localnum[i] = (fgetc(logfil));
		for (i=0; i<=43; i++) initstring[i] = (fgetc(logfil));
		for (i=0; i<=3; i++)  com_str[i] = (fgetc(logfil));
		for (i=0; i<=6; i++)  params[i] = (fgetc(logfil));
		strcat(initstring, "\r");
		fclose(logfil);
	  }
	else  {
	  printf( "%s\n", "Please run NWSCONFG before using the Noon News Retriever");
	  exit(1);
	}
	if ((logfil = fopen("noonnews.txt", "w")) == NULL)  {
		 printf( "%s\n", "Can't open noonnews.txt file!");
		 exit(1);
	 }

	time(&tt);
	ptr = localtime(&tt);
	 switch (ptr->tm_wday)	{
			case 0 : day = '5'; break;
			case 1 : day = '1'; break;
			case 2 : day = '2'; break;
			case 3 : day = '3'; break;
			case 4 : day = '4'; break;
			case 5 : day = '5'; break;
			case 6 : day = '5'; break;
		}

	news = 0;
    initvid();                                /* initialize video routines */
    if (v_mode == CO80)
    {
        cyn = CYN, hred = H_RED, grn = GRN, hgrn = H_GRN,
        wht = WHT, hblu = H_BLU, rev = RVRS;
    }
    else
    {
        cyn = WHT, hred = H_WHT, grn = WHT, hgrn = H_WHT,
	wht = WHT, hblu = H_WHT, rev = RVRS;
    }
    v_color = cyn;
    cls();                                             /* clear the screen */
    /*  D I S P L A Y   S I G N O N  */
	d_str("GM Noon News Retrieval System\n\n");
		if (!stricmp(com_str, "COM2"))  {
				IOadrs = 0x2f8; irqm = IRQ3; vctrn = VCTR3;
			 }

	/*	O P E N   T H E   C O M M	P O R T  */
	port = malloc(sizeof(ASYNC));
	AllocRingBuffer(port, RXBUFSZ, TXBUFSZ, 1);    /* alloc FAR ring bufrs */
	if ((ch = async_open(port, IOadrs, irqm, vctrn, params)) != 0)
	{
		/* couldn't open the port code */
		strsum(buf, "Couldn't open ", com_str, " using ",
			  params, " for parameters -- ABORTING\n\n", NULL);
		d_str(buf);
		strsum(buf, "Error code = ", itoa(ch, &buf[50], 10), "\n\n", NULL);
		d_str(buf);
		exit(ch);
	}

	/* opened OK code */


	d_str("Attempting to retrieve the GM Noon News - please wait.\n\n");
	*dialstr = '\0';                              /* clear out dial string */
	tickhookset(1); 	   /* enable 'ticker' variable for timer functions */
	if (!async_carrier(port))
	{
	async_tx_str(initstring);						/* reset the modem */
	waitforOK(90);
	}

delay(1000);
strsum(dialstr, "ATDT", localnum, "\r", NULL);
async_tx_str(dialstr);
delay(2000);
waitfor("CONNECT 1200",10000);
delay(4000);
async_tx(port, (char)ENTER);
async_tx(port, (char)ENTER);
waitfor("TERMINAL=",400);
d_str("Connection made, attempting to log-on. . . .\n\n");
async_tx_str("D1");
async_tx(port, (char)ENTER);
waitfor("@",400);
async_tx_str("mail");
async_tx(port, (char)ENTER);
waitfor("name?",500);
delay(500);
async_tx_str("a.923423");
delay(500);
async_tx(port, (char)ENTER);
waitfor("Password?",500);
delay(1000);
async_tx_str(logonid);
async_tx(port, (char)ENTER);

if (waitfor("Command?",2000) != 0 )  {
	hang_up();
    d_str("Logon Attempt FAILED, try again later!\n\n");
	delay(3000);
	exit_term();
   }
async_tx_str("c GM.NOON.NEWS");
async_tx(port, (char)ENTER);
waitfor("NUMBER -",800);
async_tx(port, day);
async_tx(port, (char)ENTER);
d_str("Writing GM Noon News file: noonnews.txt. . . .\n\n");
waitfor("Subj:",1800);
news = 1;
waitfor("Command?",550800);
fclose(logfil);
news = 0;
d_str("Retrieval should be Successful!\n\n");
async_tx_str("bye");
async_tx(port, (char)ENTER);
waitfor("@",1800);
exit_term();

	/*
	T H I S   I S	T H E	M A I N   I N G R E D I E N T
	*/
	while (1)
	{
		 /* check the keyboard */
		if (KBHIT)	{
			if ((char)KBREAD == ESC)
			   exit_term();
		 }
		 /* check the serial port */
		if (async_rxcnt(port))
			proc_rxch(async_rx(port));

	}
}

int proc_rxch(int ch)
{
	REG int ch2, ch3;

    ch2 = ch & 0xff;

     if (ch2 == '\r' && lfs)
          ch2 = '\n';              /* translate to nl if supplying lin feeds */

    else  {
      if (news)
         fputc((char)ch2, logfil);

     }
    return (ch);
}

/*
		w a t c h	c a r r i e r  d e t e c t
*/
int 	watch_cd(void)
{
	return(watchdogset(1, port->ComBase));
}

/*
		r e s e t	c o l o r,	 c l e a r	 s c r e e n
*/
int 	rcls(void)
{
	v_color = cyn;
	cls();
	return 0;
}

/*
		t o g g l e   e c h o	 f u n c t i o n
*/
int 	toggle_echo(void)
{
	if (echo ^= 1)	{
	  } 									  /* toggle flag */
	return 0;
}

/*
		t o g g l e   d t r    f u n c t i o n
*/
int 	toggle_dtr(void)
{
	async_dtr(port, (dtr ^= 1));							 /* toggle DTR */
	return 0;
}

/*
		e x i t   f o r   g o o d	f u n c t i o n
*/
void   exit_term()
{
	hang_up();
	tickhookset(0); 				   /* disable the TIMER interrupt hook */
	async_close(port);									 /* close the port */
	exit (0);												/* back to DOS */
}

/*
		a s y n c _ t x _ s t r   f u n c t i o n
*/
int 	async_tx_str(char *str)
{
	return(async_txblk(port, str, strlen(str)));
}

/*
		a s y n c _ t x _ e c h o	f u n c t i o n
*/
int 	async_tx_echo(register char *str)
{

	while (*str && async_carrier(port))
	{
		d_ch(*str);
		async_tx(port, *str++);
		while (!async_stat(port, B_TXEMPTY | B_CD))
			;
	}
	return 0;
}

/*
		r e c e i v e	w i t h   t i m e o u t
			Gets a char from the async port 'port'.  Times out after so many
			tenths of a second.
*/
int 	rx_timeout(int Tenths)
{
	uint StatChar;
	long rto;

	/* if char is in buffer return it with no kbd or timeout chks */
	if (!(B_RXEMPTY & (StatChar = async_rx(port))))
		return (StatChar & 0xff);

	/* if no char in rxbufr, set up timeout val & wait for char or timeout */
	SET_TO_TENTHS(rto, Tenths);
	while (1)
	{
		if (!(B_RXEMPTY & (StatChar = async_rx(port))))
			return (StatChar & 0xff);	  /* return with char if one ready */
		if (ChkgCarrier && (B_CD & StatChar))
			return (LOST_CARRIER);				 /* ret if carrier dropped */
		if (ChkgKbd && KBHIT && (char)KBREAD == ESC)
			return (ESC_PRESSED);	  /* ret if watching Kbd & ESC pressed */
		if (timed_out(&rto))
			return (TIMED_OUT); 				 /* ret if ran out of time */
	}
}

/*
		w a i t   f o r   m o d e m   t o	s e n d   O K
*/
int 	waitforOK(int ticks)
{
	char lbuf[2];

	if (waitfor("OK", ticks))
		{
		printf( "%s\n", "Modem does not respond,  Check your setup!");
		delay(3000);
		exit_term();
	   }
	return 0;
}

/*
		w a i t f o r	f u n c t i o n
*/
int 	waitfor(char *str, int ticks)
{
	REG char *p1, *lbuf;
	long	to;
	char	ch, *end;
	int 	matchlen;

	if ((matchlen = strlen(str)) == 0)
		return (0);
	set_timeout(&to, ticks);
	lbuf = calloc(matchlen + 1, 1);
	p1 = lbuf - 1;
	end = p1 + matchlen;
	while (1)
	{
		if (timed_out(&to))
		{
			free(lbuf);
			return (TIMED_OUT);
		}
		if (KBHIT && ((char)KBREAD == ESC))
		{
			free(lbuf);
			return (ESC_PRESSED);
		}
		if (!async_rxcnt(port))
			continue;
		ch = (char)proc_rxch(async_rx(port));
		if (p1 != end)
		{
			*++p1 = ch;
			if (p1 != end)
				continue;
		}
		else
		{
			memmove(lbuf, &lbuf[1], matchlen);
			*p1 = ch;
		}
		if (*lbuf == *str && !memicmp(lbuf, str, matchlen))
		{
			free(lbuf);
			return (0);
		}
	}
}

/*
		h a n g   u p	t h e	p h o n e
*/
int 	hang_up(void)
{
	long rto;

	if (dtr)
		toggle_dtr();
	async_txflush(port);
	async_rxflush(port);
	SET_TO_TENTHS(rto, 3);
	while (!timed_out(&rto))
	{
		if (!async_carrier(port))
		{
			toggle_dtr();
			return 0;
		}
	}
	toggle_dtr();
	DELAY_TENTHS(11);
	async_tx_str("+++");
	DELAY_TENTHS(15);
	async_tx_str("ATH0\r");
	waitforOK(25);
	return 0;
}

