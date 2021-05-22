/*******************************************************/
/* Full blown PartsPeed terminal					   */
/*******************************************************/
#define VER        "v0.3"

#include <stdio.h>
#include <fcntl.h>
#if !defined(__TURBOC__)
	#include <sys\types.h>
#endif
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

#include "comm.h"                            /* header for async functions */
#include "ansidrv.h"                   /* header for ANSI display routines */
#define KEY_INT
#include "keys.h"                                 /* key definition header */
#if defined(RED)
	#undef RED
#endif
#include "colors.h"                  /* vid mode & color definition header */
#include "extra.h"                               /* extra functions header */

#if defined (__TURBOC__)										/* Turbo C */
  #define	NO_FILES(x) 	findfirst((x), &findbuf, 0)
  #define	find_t			ffblk
  #define	KBHIT			bioskey(1)
  #define	KBREAD			bioskey(0)
  #include	<dir.h>
#else											 /* Microsoft C, Zortech C */
  #include	<direct.h>
  #define	NO_FILES(x) 	_dos_findfirst((x), _A_NORMAL, &findbuf)
  #define	KBHIT			_bios_keybrd(_KEYBRD_READY)
  #define	KBREAD			_bios_keybrd(_KEYBRD_READ)
#endif

#define POP 		0
#define PUSH		1			   /* used in screen save/restore function */
#define RXBUFSZ 	4096							/* rx ring buffer size */
#define TXBUFSZ 	1050							/* tx ring buffer size */
#define LOST_CARRIER -1
#define NO_INPUT	 -1
#define ESC_PRESSED  -2
#define TIMED_OUT	 -3
#define TRUE		1
#define FALSE		0

#define REG 	register

typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int uint;

extern  int acd_logo();

/* function declarations */
void       toggle_printer();
void       draw_idle_menu();
void       draw_online_menu();
void       draw_next_menu();
static int proc_keypress(int ch);
int        logon_procedure();
static int proc_rxch(int ch);
static int proc_fkey(int keycd);
static int watch_cd(void );
static int rcls(void);
static int toggle_echo(void );
static int toggle_lfs(void );
static int toggle_dtr(void );
static int exit_term(void );
static int async_tx_str(char *str);
static int async_tx_echo(char *str);
static int screen_pushpop(int flag);
static int rx_timeout(int Tenths);
static int waitfor(char *str, int ticks);
static int waitforOK(int ticks );
static int prompt(char *kbuf,char *promptmsg,int maxchars);
static int comprompt(char *kbuf,char *promptmsg,int maxchars);
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
int 	hide = 0;		  /* used to hide characters received */
int 	inq = 0;		  /* to display default order quantity when prompting */
int 	ChkgCarrier = 0;		/* monitor carrier during rcv with timeout */
char	buf[200];									 /* gen purpose buffer */
int 	lfs = 0, echo = 0, dtr = 1; 		  /* various flags */
struct	find_t findbuf; 					   /* struct for NO_FILES macro */
int 	cyn, hred, grn, hgrn, wht, hblu, rev;	   /* text color variables */
int     print_warn = 0, print_on = 0;
int     next_menu = 0;
char	localnum[19], logonid[19], initstring[49];

/*
**	main program
*/
void main()
{
	static char params[10] = "2400E71";              /* default parameters */
	char		buf[100];
	int 		chr;
	unsigned	i, ch;



    acd_logo();

    /* this reads the config file for setup parameters */
    if (NULL != (logfil = fopen("termparm.ini", "r")))  {
	   localnum[0] = logonid[0] = initstring[0] = '\0';
	   for (i=0; i<=12; i++)  {
		 chr = fgetc(logfil);
		 chr = chr >> 1;
		 logonid[i] = chr;
	   }
	   for (i=0; i<=13; i++) localnum[i] = (fgetc(logfil));
	   for (i=0; i<=43; i++) initstring[i] = (fgetc(logfil));
	   for (i=0; i<=3; i++)  com_str[i] = (fgetc(logfil));
	   for (i=0; i<=6; i++)  params[i] = (fgetc(logfil));
	   strcat(initstring, "\r");
	   fclose(logfil);
	}
	else  {
      printf( "%s\n", "Please run PSETUP.EXE before using the PartSpeed Terminal.");
	  exit(0);
	}
	logfil = NULL;

	initvid();								  /* initialize video routines */
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

/* this attempts to detect if your password has expired by looking for a */
/* a file called pspnwpsw.req.	This file was written if the last logon  */
/* attempt detected that your password had expired						 */
/*
  if (NULL != (logfil = fopen("pspnwpsw.req", "r")))  {
	   fclose(logfil);
	   prompt(buf, "System password must be updated!  Press ENTER to exit", 0);
	   exit(0);
	}
*/

  /* if your using COM2 then set the params different then default */
  if (!stricmp(com_str, "COM2"))
	 IOadrs = 0x2f8; irqm = IRQ3; vctrn = VCTR3;

  /* attempt to open the comm port */
	port = malloc(sizeof(ASYNC));
	AllocRingBuffer(port, RXBUFSZ, TXBUFSZ, 1);    /* alloc FAR ring bufrs */
	if ((ch = async_open(port, IOadrs, irqm, vctrn, params)) != 0)
	{
		/* couldn't open the port code */
		strsum(buf, "PartSpeed: Couldn't open ", com_str, " using ",
			  params, " for parameters -- ABORTING\n\n", NULL);
		d_str(buf);
		strsum(buf, "Error code = ", itoa(ch, &buf[50], 10), "\n\n", NULL);
		d_str(buf);
		exit(ch);
	}

/* if you got here then the terminal should be working okay 			 */
/*************************************************************************/

  /* display the main menu bar	*/
  v_color = H_WHT;  /* set color to very bright white */
  cls();
  SETWND(0, 0, 23, 79); /* set the window size and ensure status bar line */
  draw_idle_menu();
  /*
  **  This wile loop is what does the work on the comm port
  */
  while (1)
	{
		 /* check the keyboard */
		if (KBHIT)
			proc_keypress(KBREAD);
		 /* check the serial port */
		if (async_rxcnt(port))
			proc_rxch(async_rx(port));
	}
} /* end main */

void toggle_printer()
{
    if (!print_warn)  {
       prompt(buf, "Make sure printer is turned on!  Press ENTER", 0);
       print_warn = 1;
    }
    if (!print_on)
       print_on = TRUE;
    else
       print_on = FALSE;
}

void draw_idle_menu()
{
  d_strnat(24, 0, 'Û', hblu, 80);   /* draw a bar */
  d_msgat(24, 3, rev, "F1-Logon ³ End-Quit ³ Home-Toggle Printer");
  sprintf(buf, "%s%s%s","* PartSpeed Terminal ", VER, " *");
  d_msgat(24, 52, rev, buf);

}

void draw_online_menu()
{
  d_strnat(24, 0, 'Û', hblu, 80);   /* draw a bar */
  d_msgat(24, 1, rev, "F1-PAR ³ F2-COM ³ F3-INV ³ F4-ORD ³ F5-NEXT ³ F6-LOGOFF ³ PageDown-Next Menu ");
}

void draw_next_menu()
{
  d_strnat(24, 0, 'Û', hblu, 80);   /* draw a bar */
  d_msgat(24, 1, rev, "F1-BAC ³ F2-PAS ³ F3-PAY ³ F4-CPO ³ F5-CLM ³ F6-LOGOFF ³ PageUp-Prev Menu ");
}

int logon_procedure()
{
  cls();
  tickhookset(1);		 /* enable 'ticker' variable for timer functions */

  if (!async_carrier(port))  {
	hide = TRUE;
	d_str("Attempting to initialize the modem.\n");
	async_tx_str(initstring);						/* reset the modem */
    if (!waitforOK(100))  {
	   d_str("Modem responded!\n");
	   /* display status line */
	   d_strnat(24, 0, 'Û', wht, 80);                       /* paint a bar */
       d_msgat(24, 5, rev, "Please wait, trying to establish a connection.    ³    Esc-Cancel");
	   *dialstr = '\0';                           /* clear out dial string */
	   delay(1000);
	   strsum(dialstr, "ATDT", localnum, "\r", NULL);
	   async_tx_str(dialstr);
       d_str("Dialing ");
       d_str(localnum);
	   delay(2000);
	   if (!waitfor("CONNECT",500))  {    /* was 10000 */
		 delay(4000);
         d_str("\nGot a connect signal.\n");
		 async_tx(port, (char)ENTER);
		 async_tx(port, (char)ENTER);
		 waitfor("TERMINAL=",400);
		 d_strnat(24, 0, 'Û', wht, 80);                          /* paint a bar */
		 d_msgat(24, 5, rev, "Connection made, attempting to logon. . . .");
		 d_str("Issuing terminal type.\n");
		 async_tx_str("D1");
		 async_tx(port, (char)ENTER);
		 waitfor("@",400);
		 d_str("Issuing PartSpeed address.\n");
		 async_tx_str("2291140");
		 async_tx(port, (char)ENTER);
		 waitfor("CONNECTED",500);
		 delay(2000);
		 async_tx_str("DELCO");
		 delay(2000);
		 async_tx(port, (char)ENTER);
		 waitfor("Z",500);
		 delay(1000);
		 d_str("Issuing account and password.\n");
		 async_tx_str(logonid);
		 async_tx(port, (char)ENTER);
	  /*
		 if (waitfor("INVALID PASSWORD)",50) != 0 )  {
			SETWND(0, 0, 24, 79);
			cls();
			logfil = fopen("pspnwpsw.req", "w");
			fclose(logfil);
			hang_up();
			prompt(buf, "System password must be updated!  Press ENTER to exit", 0);
			exit_term();
		 }
	  */
		 if (waitfor("T)",80) != 0 )  {
			hang_up();
			prompt(buf, "Logon attempt Unsuccesful!  Press ENTER to exit", 0);
			exit_term();
		 }
	   hide = FALSE;
	   cls();
	   async_tx_str("MENU");
	   async_tx(port, (char)ENTER);
       draw_online_menu();
       return (1);
	 }
	 else { /* wait for connect */
	   hide = FALSE;
	   cls();
	   d_str("Sorry, connection failed!  Try again....\n");
       draw_idle_menu();
	 }
   }
   else
	  hide = FALSE;
 }

}

int proc_rxch(int ch)
{
	REG int ch2;

	ch2 = ch & 0xff;

	if (ch2 == '\r' && lfs)
	   ch2 = '\n';              /* translate to nl if supplying lin feeds */
	 if (!hide) 				/* Can hide received chars if hide==1	  */
	  d_ch((char)ch2);			/* dsply if got one -- 'd_ch' supports ANSI codes */

    if (print_on && !v_ansiseq)
        fputc(ch2, stdprn);     /* write char to printer if toggled on */
	return (ch);
}

/*
		p r o c e s s	k e y p r e s s
*/
int proc_keypress(REG int ch)
{
	REG int ch2;

	if ((ch2 = ch & 0xff) == 0)
		proc_fkey(ch);					/* process if extended key pressed */
	else
	{
	async_tx(port, (char)ch2);							/* send ch to port */
		if (echo && ch != X_ESC)
		{
			if (ch2 == '\r' && lfs)
				ch2 = '\n';                  /* make it a nl if adding lfs */
		d_ch((char)ch2);							   /* display the char */
			if (logfil && !v_ansiseq)
				fputc(ch2, logfil); 		   /* write to logfile if open */
		}
	}
	return (ch);
}

/*
		p r o c e s s	e x t e n d e d   k e y s
*/
int    proc_fkey(REG int keycd)
{

 if (!async_carrier(port))  {
	switch(keycd)
	 {
      case F1 : logon_procedure(); break;
	  case F4 : async_tx(port, (char)ENTER); break;
     case END : exit_term(); break;
     case HOME: toggle_printer(); break;
   case ALT_C : cls(); break;
	  default : return (0);
	 }						  /* else do nothing and return a 0 */
 }
 else  {
    switch(keycd)
	 {
      case F1 :  if (!next_menu)
                    async_tx_str("PAR\r");
                 else
                    async_tx_str("BAC\r");
                break;
      case F2 : if (!next_menu)
                   async_tx_str("COM\r");
                else
                   async_tx_str("PAS\r");
                break;
      case F3 : if (!next_menu)
                   async_tx_str("INV\r");
                else
                   async_tx_str("PAY\r");
                break;
      case F4 : if (!next_menu)
                   async_tx_str("ORD\r");
                else
                   async_tx_str("CPO\r");
                break;
      case F5 : if (!next_menu)
                   async_tx_str("NEXT\r");
                else
                   async_tx_str("CLM\r");
                break;
      case F6 : async_tx_str("EXIT\r");
				waitfor("@",1000);
				async_tx_str("hangup\r");
				waitfor("CARRIER",1000);
                draw_idle_menu();
                break;
  case  ALT_H : hang_up();
                draw_idle_menu();
                break;
   case  PGUP : draw_online_menu();
                next_menu = FALSE;
                break;
   case  PGDN : draw_next_menu();
                next_menu = TRUE;
                break;
   case ALT_C : cls();
                break;
    case HOME : toggle_printer();
                break;
      default : return (0);
     }
  }
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
		t o g g l e   l f ' s    f u n c t i o n
*/
int 	toggle_lfs(void)
{
	if (lfs ^= 1) {
	 }											 /* toggle flag */
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
int 	exit_term(void)
{
	char	lbuf[2];

	switch (prompt(lbuf, "Quit PartSpeed Terminal (Y or N)?", 1))
	{
	  case ESC_PRESSED:
		return 0;
	  case NO_INPUT:
		break;
	  default:
		if (*lbuf != 'Y' && *lbuf != 'y')
			return 0;
	}
	tickhookset(0); 				   /* disable the TIMER interrupt hook */
	async_close(port);									 /* close the port */
	SETWND(0, 0, 24, 79);
    v_color = wht; /* set the color back to normal */
    cls();                                             /* clear the screen */
    printf("%s %s %s\n", "PartSpeed Terminal", VER, "Copyright 1996");
    printf("%s\n", "Written by Fritz Feuerbacher");
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
		s c r e e n   s a v e / r e s t o r e	f u n c t i o n
*/
int 	screen_pushpop(int flag)
{
	static char *savscrn = NULL;
	static ulong   savdwndsz;
	static int	   x;

	if (flag == PUSH)
	{
		x = v_ansiseq, v_ansiseq = 0;/* in case an ANSI seq is in progress */
	if ((savscrn = malloc(SCRBUF(25, 80))) != NULL)
		{
			savdwndsz = v_wndsz;
			SETWND(0, 0, 24, 79);		  /* reset window to entire screen */
			pu_scrnd(0, 0, 25, 80, savscrn);/* save current screen in bufr */
			return (1); 								 /* return success */
		}
		else
			return (0); 								  /* return failed */
	}
	else
	{
		v_ansiseq = x;								/* restore ANSI status */
		if (savscrn != NULL)
		{
			po_scrnd(savscrn);				  /* restore the pushed screen */
			free(savscrn);				   /* release screen buffer memory */
			savscrn = NULL;
			v_wndsz = savdwndsz;				  /* reset the screen size */
			return (1); 								 /* return success */
		}
		else
			return (0); 				/* return failed if nothing pushed */
	}
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

	if (waitfor("OK", ticks))  {
		prompt(lbuf, "Modem does not respond.  Check COM port# and modem power!\a", 0);
		exit_term();
		cls();
		return 1;
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
		if (KBHIT && proc_keypress(KBREAD) == X_ESC)
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
		p r o m p t   f o r   k e y b o a r d	i n p u t
*/
int 	prompt(char *kbuf, char *promptmsg, int maxchars)
{
	REG 	char *p1;
	char	ch = 0, *scrnbuf, *endkbuf;
	int 	i, x, y, boxtop, boxlft, boxlen;

	x = v_ansiseq, v_ansiseq = 0;				 /* save ANSI seq variable */
	y = v_color, v_color = hblu;				   /* set the color to use */
	boxlen = strlen(promptmsg) + maxchars + 6;
	boxlft = (80 - boxlen) / 2;
	boxtop = (v_btm - 5) / 2;				   /* calculate box dimensions */
	scrnbuf = malloc(SCRBUF(5, boxlen));
	pu_scrnd(boxtop, boxlft, 5, boxlen, scrnbuf);  /* save the screen area */
	for (i = 1; i < 4; i++)
		d_nchat(boxtop + i, boxlft + 1, ' ', wht, boxlen - 2, HORZ);
	d_chat(boxtop, boxlft, 'É');
	d_nchat(boxtop, boxlft + 1, 'Í', hblu, boxlen - 2, HORZ);
	d_chat(boxtop, boxlft + boxlen - 1, '»');
	d_nchat(boxtop + 1, boxlft, 'º', hblu, 3, VERT);
	d_chat(boxtop + 4, boxlft, 'È');
	d_nchat(boxtop + 4, boxlft + 1, 'Í', hblu, boxlen - 2, HORZ);
	d_chat(boxtop + 4, boxlft + boxlen - 1, '¼');            /* draw a box */
	d_nchat(boxtop + 1, boxlft + boxlen - 1, 'º', hblu, 3, VERT);
	d_msgat(boxtop + 2, boxlft + 3, hred, promptmsg);
	loc(boxtop + 2, boxlft + strlen(promptmsg) + 4); /* display the prompt */
	v_color = wht;
	 if (inq == 1)	{
		d_ch('1');
		loc(boxtop + 2, boxlft + strlen(promptmsg) + 4); /* disp default qty */
	  }
	for (p1 = kbuf, endkbuf = p1 + maxchars; ch != '\r' && ch != ESC;)
	{
		ch = KBREAD & 0xff; 								 /* get a char */
		if (ch != '\r' && ch != ESC)
		{
			if (ch == '\b')
			{
				if (p1 > kbuf)				   /* backspace key processing */
					--p1, d_ch(ch);
				continue;
			}
			else if (p1 != endkbuf && isprint(ch))
			{
			   if (ch == ' ')  {
				  *p1++ = '-';
				  d_ch('-');
				 }
			   else  {
				 d_ch(toupper(ch)), *p1++ = toupper(ch);	   /* put char in buffer */
				}
				continue;
			}
			d_ch('\a');               /* beep if max chars already entered */
		}
	 if (strcmp(p1,"1"))
		*p1 = '\0';                                /* terminate the buffer */
	}
	po_scrnd(scrnbuf);
	free(scrnbuf);
	v_ansiseq = x, v_color = y; 			 /* restore screen & variables */
	if (ch == ESC)
		return (ESC_PRESSED);				 /* return this if ESCaped out */
	if (p1 == kbuf)
		return (NO_INPUT);					   /* return val if ENTER only */
	return (0); 						   /* return val if got some input */
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

