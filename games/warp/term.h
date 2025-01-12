/* Header: term.h,v 7.0.1.2 86/12/12 17:05:15 lwall Exp */

/* Log:	term.h,v
 * Revision 7.0.1.2  86/12/12  17:05:15  lwall
 * Baseline for net release.
 * 
 * Revision 7.0.1.1  86/10/16  10:53:33  lwall
 * Added Damage.  Fixed random bugs.
 * 
 * Revision 7.0  86/10/08  15:14:07  lwall
 * Split into separate files.  Added amoebas and pirates.
 * 
 */

#ifndef TERM_H
#define TERM_H

#include "util.h"

/* Compat with old termios. */
#ifndef ECHO
#define ECHO 8
#endif

/* warp will still work without the following, but may get ahead at low speed */
#ifdef TIOCOUTQ		/* chars left in output queue */
#define output_pending() (ioctl(1, TIOCOUTQ, &iocount),iocount)
#endif

/* If some of the following look something like curses calls, it is because
 * warp used to use curses but doesn't now.  Warp was neither as efficient nor
 * as portable with curses, and since the program had to cheat on curses all
 * over the place anyway, we ripped it out.
 */
#define setimage(of,to) (mvaddch(of->posy+1,of->posx*2,of->image=(to)))

#define mvaddch(y,x,ch) move((y),(x),(ch))
/* #define addch(ch) (tmpchr=(ch), write(1,&tmpchr,1), real_x++) */
#define mvaddc(y,x,ch) move((y),(x),(ch))
#define addc(ch) (write(1,&(ch),1), real_x++)
#define addspace() (write(1," ",1), real_x++)
#define mvaddstr(y,x,s) (move((y),(x),0), tmpstr = (s), \
     tmplen = strlen(tmpstr), write(1, tmpstr, tmplen), real_x += tmplen)

EXT size_t tmplen;
EXT const char *tmpstr;
/* EXT char tmpchr; */

/* The following macros are like the pseudo-curses macros above, but do
 * certain amount of controlled output buffering.
 *
 * NOTE: a beg_qwrite()..end_qwrite() sequence must NOT contain a cursor
 * movement (move), because the move() routine uses beg_qwrite()..end_qwrite()
 * itself.
 */

#define beg_qwrite() (maxcmstring = cmbuffer)
#ifdef vax
#define qwrite() asm("movc3 _gfillen,_filler,*_maxcmstring"); maxcmstring += gfillen
#else
#define qwrite() (movc3(gfillen,filler,maxcmstring), maxcmstring += gfillen)
#endif
#define qaddc(ch) (*maxcmstring++ = (ch), real_x++)
#define qaddch(ch) (*maxcmstring++ = (ch), real_x++)
#define qaddspace() (*maxcmstring++ = ' ', real_x++)
#define end_qwrite() (write(1,cmbuffer,maxcmstring-cmbuffer))

/* setting a ??size to infinity forces cursor addressing in that direction */

EXT int CMsize;
EXT int BCsize INIT(1);
EXT int DOsize INIT(1000);
EXT int UPsize INIT(1000);
EXT int NDsize INIT(1000);

EXT int charsperhalfsec;

EXT int real_y INIT(-100);
EXT int real_x INIT(-100);

#ifdef DOINIT
char filler[] = {0,'\b',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#else
EXT char filler[];
#endif

EXT char *bsptr INIT(filler+1);

EXT char term[12];

EXT char gfillen INIT(25);

EXT char *maxcmstring;
EXT char cmbuffer[512];

#define BREAKCH '\0'

EXT char INTRCH INIT('\03');

#ifdef PUSHBACK
    EXT char circlebuf[PUSHSIZE];
    EXT int nextin INIT(0);
    EXT int nextout INIT(0);
#   ifdef PENDING
#	ifdef FIONREAD
	    EXT long iocount INIT(0);
#	    ifndef lint
#		define input_pending() (nextin!=nextout || \
(ioctl(0, FIONREAD, &iocount),(int)iocount))
#	    else
#		define input_pending() bizarre
#	    endif /* lint */
#	else /* FIONREAD */
	    int circfill();
#	    ifdef RDCHK
#		ifndef lint
#		    define input_pending() rdchk(0)
#		else /* lint */
#		    define input_pending() bizarre
#		endif /* lint */
#	    else /* RDCHK */
#		ifndef O_NDELAY	/* assert O_NDELAY */
		    ??? PENDING is not defined correctly in warp.h
#		endif
		EXT int devtty INIT(0);
#		ifndef lint
#		    define input_pending() (nextin!=nextout || circfill())
#		else
#		    define input_pending() bizarre
#		endif /* lint */
#	    endif /* RDCHK */
#	endif /* FIONREAD */
#   else /* PENDING */
#	??? warp will not work without PENDING
#	ifndef lint
#	    define input_pending() (nextin!=nextout)
#	else
#	    define input_pending() bizarre
#	endif /* lint */
#   endif /* PENDING */
#else /* PUSHBACK */
#   ifdef PENDING
#	ifdef FIONREAD /* must have FIONREAD or O_NDELAY for input_pending() */
#	    define read_tty(addr,size) read(0,addr,size)
#	    ifndef lint
#		define input_pending() (ioctl(0, FIONREAD, &iocount), \
(int)iocount)
#	    else
#		define input_pending() bizarre
#	    endif /* lint */
	    EXT long iocount INIT(0);
#	else /* FIONREAD */
#	    ifdef RDCHK		/* actually, they can have rdchk() too */
#	    define read_tty(addr,size) read(0,addr,size)
#		ifndef lint
#		    define input_pending() rdchk(0)
#		else /* lint */
#		    define input_pending() bizarre
#		endif /* lint */
#	    else /* RDCHK */
#		ifndef O_NDELAY	/* assert O_NDELAY */
		    ??? PENDING is not defined correctly in warp.h
#		endif
		EXT int devtty INIT(0);
		EXT bool is_input INIT(false);
		EXT char pending_ch INIT(0);
#		ifndef lint
#		    define input_pending() (is_input || \
(is_input=read(devtty,&pending_ch,1)))
#		else
#		    define input_pending() bizarre
#		endif /* lint */
#	    endif /* RDCHK */
#	endif /* FIONREAD */
#   else /* PENDING */
	??? warp will not work without PENDING
#	define read_tty(addr,size) read(0,addr,size)
#	define input_pending() (false)
#   endif /* PENDING */
#endif /* PUSHBACK */

/* stuff wanted by terminal mode diddling routines */

#ifdef TERMIOS
EXT struct termios _tty, _oldtty;
#elif defined(TERMIO)
typedef int speed_t;
EXT struct termio _tty, _oldtty;
#define tcsetattr(fd, how, ti) ioctl(fd, how, ti)
#define tcgetattr(fd, ti) ioctl(fd, TCGETA, ti)
#define cfgetospeed(ti) ((ti)->c_cflag & CBAUD)
#else
typedef int speed_t;
EXT struct sgttyb _tty;
EXT int _res_flg INIT(0);
#endif

EXT int _tty_ch INIT(2);
EXT bool bizarre INIT(false);			/* do we need to restore terminal? */

/* terminal mode diddling routines */

#if defined(TERMIO) || defined(TERMIOS)
  
#define raw() ((bizarre=1),_tty.c_lflag &=~ISIG,_tty.c_cc[VMIN] = 1,tcsetattr(_tty_ch,TCSAFLUSH,&_tty))
#define noraw() ((bizarre=1),_tty.c_lflag |= ISIG,_tty.c_cc[VEOF] = CEOF,tcsetattr(_tty_ch,TCSAFLUSH,&_tty))
#define crmode() ((bizarre=1),_tty.c_lflag &=~ICANON,_tty.c_cc[VMIN] = 1,tcsetattr(_tty_ch,TCSAFLUSH,&_tty))
#define nocrmode() ((bizarre=1),_tty.c_lflag |= ICANON,_tty.c_cc[VEOF] = CEOF,tcsetattr(_tty_ch,TCSAFLUSH,&_tty))
#define echo()	 ((bizarre=1),_tty.c_lflag |= ECHO, tcsetattr(_tty_ch, TCSANOW, &_tty))
#define noecho() ((bizarre=1),_tty.c_lflag &=~ECHO, tcsetattr(_tty_ch, TCSANOW, &_tty))
#define nl()	 ((bizarre=1),_tty.c_iflag |= ICRNL,_tty.c_oflag |= ONLCR,tcsetattr(_tty_ch, TCSANOW, &_tty))
#define nonl()	 ((bizarre=1),_tty.c_iflag &=~ICRNL,_tty.c_oflag &=~ONLCR,tcsetattr(_tty_ch, TCSANOW, &_tty))
#define	savetty() (tcgetattr(_tty_ch, &_oldtty),tcgetattr(_tty_ch, &_tty))
#define	resetty() ((bizarre=0),tcsetattr(_tty_ch, TCSAFLUSH, &_oldtty))
#define unflush_output()

#else

#define raw()	 ((bizarre=1),_tty.sg_flags|=RAW, stty(_tty_ch,&_tty))
#define noraw()	 ((bizarre=1),_tty.sg_flags&=~RAW,stty(_tty_ch,&_tty))
#define crmode() ((bizarre=1),_tty.sg_flags |= CBREAK, stty(_tty_ch,&_tty))
#define nocrmode() ((bizarre=1),_tty.sg_flags &= ~CBREAK,stty(_tty_ch,&_tty))
#define echo()	 ((bizarre=1),_tty.sg_flags |= ECHO, stty(_tty_ch, &_tty))
#define noecho() ((bizarre=1),_tty.sg_flags &= ~ECHO, stty(_tty_ch, &_tty))
#define nl()	 ((bizarre=1),_tty.sg_flags |= CRMOD,stty(_tty_ch, &_tty))
#define nonl()	 ((bizarre=1),_tty.sg_flags &= ~CRMOD, stty(_tty_ch, &_tty))
#define	savetty() (gtty(_tty_ch, &_tty), _res_flg = _tty.sg_flags)
#define	resetty() ((bizarre=0),_tty.sg_flags = _res_flg, stty(_tty_ch, &_tty))
#endif /* TERMIO */

#ifdef TIOCSTI
#ifdef lint
#define forceme(c) ioctl(_tty_ch,TIOCSTI,Null(long*))	/* ghad! */
#else
#define forceme(c) ioctl(_tty_ch,TIOCSTI,c) /* pass character in " " */
#endif /* lint */
#else
#define forceme(c)
#endif

/* termcap stuff */

/*
 * NOTE: if you don't have termlib you'll have to define these strings,
 *    the tputs routine, and the tgoto routine.
 * The tgoto routine simply produces a cursor addressing string for a given
 * x and y.  The 1st argument is a generic string to be interpreted.
 * If you are hardwiring it you might just ignore the 1st argument.
 * The tputs routine interprets any leading number as a padding factor, possibly
 * scaled by the number of lines (2nd argument), puts out the string (1st arg)
 * and the padding using the routine specified as the 3rd argument.
 */

#ifdef HAVETERMLIB
#if 0
EXT char *BC INIT(NULL);		/* backspace character */
EXT char *UP INIT(NULL);		/* move cursor up one line */
#endif
EXT char *myUP;
EXT char *ND INIT(NULL);		/* non-destructive cursor right */
EXT char *myND;
EXT char *DO INIT(NULL);		/* move cursor down one line */
EXT char *myDO;
EXT char *CR INIT(NULL);		/* get to left margin, somehow */
EXT char *VB INIT(NULL);		/* visible bell */
EXT char *CL INIT(NULL);		/* home and clear screen */
EXT char *CE INIT(NULL);		/* clear to end of line */
EXT char *CM INIT(NULL);		/* cursor motion -- PWP */
EXT char *HO INIT(NULL);		/* home cursor -- PWP */
EXT char *CD INIT(NULL);		/* clear to end of display -- PWP */
EXT char *SO INIT(NULL);		/* begin standout mode */
EXT char *SE INIT(NULL);		/* end standout mode */
EXT int SG INIT(0);		/* blanks left by SO and SE */
EXT char *US INIT(NULL);		/* start underline mode */
EXT char *UE INIT(NULL);		/* end underline mode */
EXT char *UC INIT(NULL);		/* underline a character, if that's how it's done */
EXT int UG INIT(0);		/* blanks left by US and UE */
EXT bool AM INIT(false);		/* does terminal have automatic margins? */
EXT bool XN INIT(false);		/* does it eat 1st newline after automatic wrap? */
#if 0
EXT char PC INIT(0);		/* pad character for use by tputs() */
EXT short ospeed INIT(0);	/* terminal output speed, for use by tputs() */
#endif
EXT int LINES INIT(0), COLS INIT(0);	/* size of screen */
EXT int just_a_sec INIT(960);			/* 1 sec at current baud rate */
					/* (number of nulls) */
EXT char ERASECH;		/* rubout character */
EXT char KILLCH;		/* line delete character */

/* define a few handy macros */

#define clear() (do_tc(CL,LINES),real_y=real_x=0)
#define erase_eol() do_tc(CE,1)
#define backspace() (do_tc(BC,0),real_x--)
#define clear_rest() do_tc(CD,LINES)
#define underline() do_tc(US,1)
#define un_underline() do_tc(UE,1)
#define underchar() do_tc(UC,0)
#define standout() do_tc(SO,1)
#define un_standout() do_tc(SE,1)
#define up_line() do_tc(UP,1)
#define dingaling() do_tc(VB,1)
#else
  ????????		/* up to you */
#endif

void term_init(void);
void term_set(char *);
#ifdef PUSHBACK
void pushchar(int);
void mac_init(char *);
void mac_line(char *, char *, size_t);
#endif
void page(const char *filename, size_t);
void move(int, int, int);
void do_tc(const char *, int);
int comp_tc(char *, const char *, int);
void helper(void);
void rewrite(void);
int cmstore(int);
void eat_typeahead(void);
void settle_down(void);
#ifndef read_tty
int read_tty(char *, ssize_t);
#endif
int read_nd(char *, size_t);
void getcmd(char *);
void pushstring(char *);

#endif
