char *copyright =
    "Copyright (c) 1984 through 1996, William LeFebvre";

/*
 *  Top users/processes display for Unix
 *  Version 3
 *
 *  This program may be freely redistributed,
 *  but this entire comment MUST remain intact.
 *
 *  Copyright (c) 1984, 1989, William LeFebvre, Rice University
 *  Copyright (c) 1989, 1990, 1992, William LeFebvre, Northwestern University
 */

/*
 *  See the file "Changes" for information on version-to-version changes.
 */

/*
 *  This file contains "main" and other high-level routines.
 */

/*
 * The following preprocessor variables, when defined, are used to
 * distinguish between different Unix implementations:
 *
 *	SIGHOLD  - use SVR4 sighold function when defined
 *	SIGRELSE - use SVR4 sigrelse function when defined
 *	FD_SET   - macros FD_SET and FD_ZERO are used when defined
 */

#include "os.h"
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <nl_types.h>
#include <i18n_capable.h>
#include <msgs/uxeoe.h>


/* includes specific to top */
#include "display.h"		/* interface to display package */
#include "screen.h"		/* interface to screen package */
#include "top.h"
#include "top.local.h"
#include "boolean.h"
#include "machine.h"
#include "utils.h"


nl_catd  catd=0;

/* Size of the stdio buffer given to stdout */
#define Buffersize	2048

/* The buffer that stdio will use */
char stdoutbuf[Buffersize];

/* build Signal masks */
#define Smask(s)	(1 << ((s) - 1))

/* for system errors */
extern int errno;

/* for getopt: */
extern int  optind;
extern char *optarg;

/* imported from screen.c */
extern int overstrike;

/* signal handling routines */
sigret_t leave();
sigret_t onalrm();
sigret_t tstop();
#ifdef SIGWINCH
sigret_t mywinch();	/* Ariel: curses name space pollution: winch->mywinch */
#endif

/* internal routines */
void quit();

/* values which need to be accessed by signal handlers */
static int max_topn;		/* maximum displayable processes */

/* miscellaneous things */
char *myname = "top";
jmp_buf jmp_int;
char do_threads = No;


/*
 * routines that don't return int and are not declared in standard headers
 */

char *username();
char *kill_procs();
char *renice_procs();
void reset_display();

#ifdef ORDER
extern int (*proc_compares[])();
#else
extern int proc_compare();
#endif

caddr_t get_process_info();

/* different routines for displaying the user's identification */
/* (values assigned to get_userid) */
char *username();
char *itoa7();

/* display routines that need to be predeclared */
int i_loadave();
int u_loadave();
int i_procstates();
int u_procstates();
int i_cpustates();
int u_cpustates();
int i_memory();
int u_memory();
int i_message();
int u_message();
int i_header();
int u_header();
int i_process();
int u_process();

/* pointers to display routines */
int (*d_loadave)() = i_loadave;
int (*d_procstates)() = i_procstates;
int (*d_cpustates)() = i_cpustates;
int (*d_memory)() = i_memory;
int (*d_message)() = i_message;
int (*d_header)() = i_header;
int (*d_process)() = i_process;

extern char **saved_argv;

main(argc, argv)

int  argc;
char *argv[];

{
    int i;
    int active_procs;
    int change;

    struct system_info system_info;
    struct statics statics;
    caddr_t processes;

    static char tempbuf1[50];
    static char tempbuf2[50];
    int old_sigmask;		/* only used for BSD-style signals */
    int topn = Default_TOPN;
    int delay = Default_DELAY;
    int displays = 0;		/* indicates unspecified */
    time_t curr_time;
    char *(*get_userid)() = username;
    char *uname_field = "USERNAME";
    char *header_text;
    char *env_top;
    char **preset_argv;
    int  preset_argc = 0;
    char **av;
    int  ac;
    /* char dostates = No; */
    char do_unames = Yes;
    char interactive = Maybe;
    char warnings = 0;
#if Default_TOPN == Infinity
    char topn_specified = No;
#endif
    char ch;
    char *iptr;
    char no_command = 1;
    struct timeval timeout;
    struct process_select ps;
#ifdef ORDER
    char *order_name = NULL;
    int order_index = 0;
#endif
#ifndef FD_SET
    /* FD_SET and friends are not present:  fake it */
    typedef int fd_set;
#define FD_ZERO(x)     (*(x) = 0)
#define FD_SET(f, x)   (*(x) = f)
#endif
    fd_set readfds;

#ifdef ORDER
    static char command_chars[] = "\n\f qh?en#sdkriIuSo";
#else
    static char command_chars[] = "\n\f qh?en#sdkriIuS";
#endif
/* these defines enumerate the "strchr"s of the commands in command_chars */
#define CMD_noop	0
#define CMD_redraw	1
#define CMD_update	2
#define CMD_quit	3
#define CMD_help1	4
#define CMD_help2	5
#define CMD_OSLIMIT	5    /* terminals with OS can only handle commands */
#define CMD_errors	6    /* less than or equal to CMD_OSLIMIT	   */
#define CMD_number1	7
#define CMD_number2	8
#define CMD_delay	9
#define CMD_displays	10
#define CMD_kill	11
#define CMD_renice	12
#define CMD_idletog     13
#define CMD_idletog2    14
#define CMD_user	15
#define CMD_systemtog	16
#ifdef ORDER
#define CMD_order       17
#endif

   (void) setlocale(LC_ALL, "");
   catd=catopen("uxeoe",0);
    /*
     * Security: opening /dev/kmem is the only thing that needs setgid
     * in top. We do it first and drop all privileges (setgid sys)
     * before everything else.
     */
    open_kmem();

    /* set the buffer for stdout */
#ifdef DEBUG
    setbuffer(stdout, NULL, 0);
#else
    setbuffer(stdout, stdoutbuf, Buffersize);
#endif
    saved_argv = argv;

    /* get our name */
    if (argc > 0)
    {
	if ((myname = strrchr(argv[0], '/')) == 0)
	{
	    myname = argv[0];
	}
	else
	{
	    myname++;
	}
    }

    /* initialize some selection options */
    ps.idle    = No;
    ps.system  = Yes;
    ps.uid     = -1;
    ps.command = NULL;

    /* get preset options from the environment */
    if ((env_top = getenv("TOP")) != NULL)
    {
	av = preset_argv = argparse(env_top, &preset_argc);
	ac = preset_argc;

	/* set the dummy argument to an explanatory message, in case
	   getopt encounters a bad argument */
	preset_argv[0] = "while processing environment";
    }

    /* process options */
    do {
	/* if we're done doing the presets, then process the real arguments */
	if (preset_argc == 0)
	{
	    ac = argc;
	    av = argv;

	    /* this should keep getopt happy... */
	    optind = 1;
	}

	while ((i = getopt(ac, av, "SIbinqus:d:U:o:T")) != EOF)
	{
	    switch(i)
	    {
	      case 'u':			/* toggle uid/username display */
		do_unames = !do_unames;
		break;

	      case 'U':			/* display only username's processes */
		if ((ps.uid = userid(optarg)) == -1)
		{
		    fprintf(stderr, CATGETS(catd, _MSG_UNKNOWN_USER), optarg);
		    exit(1);
		}
		break;

	      case 'S':			/* show system processes */
		ps.system = !ps.system;
		break;

	      case 'I':                   /* show idle processes */
		ps.idle = !ps.idle;
		break;

	      case 'i':			/* go interactive regardless */
		interactive = Yes;
		break;

	      case 'n':			/* batch, or non-interactive */
	      case 'b':
		interactive = No;
		break;

	      case 'd':			/* number of displays to show */
		if ((i = atoiwi(optarg)) == Invalid || i == 0)
		{
		    fprintf(stderr,
			CATGETS(catd, _MSG_TOP_DISPLAY_COUNT), myname);
		    warnings++;
		}
		else
		{
		    displays = i;
		}
		break;

	      case 's':
		if ((delay = atoi(optarg)) < 0)
		{
		    fprintf(stderr,
			CATGETS(catd, _MSG_TOP_SECONDS_DELAY), myname);
		    delay = Default_DELAY;
		    warnings++;
		}
		break;

	      case 'q':		/* be quick about it */
		/* only allow this if user is really root */
		if (getuid() == 0)
		{
		    /* be very un-nice! */
		    (void) nice(-20);
		}
		else
		{
		    fprintf(stderr,
			CATGETS(catd, _MSG_TOP_ROOT_OPTION), myname);
		    warnings++;
		}
		break;

	      case 'o':		/* select sort order */
#ifdef ORDER
		order_name = optarg;
#else
		fprintf(stderr,
			CATGETS(catd, _MSG_TOP_PLATFORM_ORDERING), myname);
		warnings++;
#endif
		break;

	      case 'T':		/* show threads */
		do_threads++;
		break;

	      default:
		fprintf(stderr, CATGETS(catd, _MSG_TOP_USAGE),
			version_string(), myname);
		exit(1);
	    }
	}

	/* get count of top processes to display (if any) */
	if (optind < ac)
	{
	    if ((topn = atoiwi(av[optind])) == Invalid)
	    {
		fprintf(stderr,
			CATGETS(catd, _MSG_TOP_PROCESS_COUNT), myname);
		warnings++;
	    }
#if Default_TOPN == Infinity
            else
	    {
		topn_specified = Yes;
	    }
#endif
	}

	/* tricky:  remember old value of preset_argc & set preset_argc = 0 */
	i = preset_argc;
	preset_argc = 0;

    /* repeat only if we really did the preset arguments */
    } while (i != 0);

    /* set constants for username/uid display correctly */
    if (!do_unames)
    {
	uname_field = "   UID  ";
	get_userid = itoa7;
    }

    /* initialize the kernel memory interface */
    if (machine_init(&statics) == -1)
    {
	exit(1);
    }

#ifdef ORDER
    /* determine sorting order index, if necessary */
    if (order_name != NULL)
    {
	if ((order_index = string_index(order_name, statics.order_names)) == -1)
	{
	    char **pp;

	    fprintf(stderr, CATGETS(catd, _MSG_TOP_SORT_ORDER),
		    myname, order_name);
	    fprintf(stderr, CATGETS(catd, _MSG_TOP_TRY_THESE));
	    pp = statics.order_names;
	    while (*pp != NULL)
	    {
		fprintf(stderr, " %s", *pp++);
	    }
	    fputc('\n', stderr);
	    exit(1);
	}
    }
#endif

#ifdef no_initialization_needed
    /* initialize the hashing stuff */
    if (do_unames)
    {
	init_hash();
    }
#endif

    /* initialize termcap */
    init_termcap(interactive);

    /* get the string to use for the process area header */
    header_text = format_header(uname_field);

    /* initialize display interface */
    if ((max_topn = display_init(&statics)) == -1)
    {
	fprintf(stderr, CATGETS(catd, _MSG_CANNOT_ALLOC_SUFF_MEM), myname);
	exit(4);
    }
    
    /* print warning if user requested more processes than we can display */
    if (topn > max_topn)
    {
	fprintf(stderr,
		CATGETS(catd, _MSG_TOP_TERMINAL_PROCESSES),
		myname, max_topn);
	warnings++;
    }

    /* adjust for topn == Infinity */
    if (topn == Infinity)
    {
	/*
	 *  For smart terminals, infinity really means everything that can
	 *  be displayed, or Largest.
	 *  On dumb terminals, infinity means every process in the system!
	 *  We only really want to do that if it was explicitly specified.
	 *  This is always the case when "Default_TOPN != Infinity".  But if
	 *  topn wasn't explicitly specified and we are on a dumb terminal
	 *  and the default is Infinity, then (and only then) we use
	 *  "Nominal_TOPN" instead.
	 */
#if Default_TOPN == Infinity
	topn = smart_terminal ? Largest :
		    (topn_specified ? Largest : Nominal_TOPN);
#else
	topn = Largest;
#endif
    }

    /* set header display accordingly */
    display_header(topn > 0);

    /* determine interactive state */
    if (interactive == Maybe)
    {
	interactive = smart_terminal;
    }

    /* if # of displays not specified, fill it in */
    if (displays == 0)
    {
	displays = smart_terminal ? Infinity : 1;
    }

    /* hold interrupt signals while setting up the screen and the handlers */
#ifdef SIGHOLD
    sighold(SIGINT);
    sighold(SIGQUIT);
    sighold(SIGTSTP);
#else
    old_sigmask = sigblock(Smask(SIGINT) | Smask(SIGQUIT) | Smask(SIGTSTP));
#endif
    init_screen();
    (void) signal(SIGINT, leave);
    (void) signal(SIGQUIT, leave);
    (void) signal(SIGTSTP, tstop);
#ifdef SIGWINCH
    (void) signal(SIGWINCH, mywinch);
#endif
#ifdef SIGRELSE
    sigrelse(SIGINT);
    sigrelse(SIGQUIT);
    sigrelse(SIGTSTP);
#else
    (void) sigsetmask(old_sigmask);
#endif
    if (warnings)
    {
	fputs("....", stderr);
	fflush(stderr);			/* why must I do this? */
	sleep((unsigned)(3 * warnings));
	fputc('\n', stderr);
    }

    /* setup the jump buffer for stops */
    if (setjmp(jmp_int) != 0)
    {
	/* control ends up here after an interrupt */
	reset_display();
    }

    /*
     *  main loop -- repeat while display count is positive or while it
     *		indicates infinity (by being -1)
     */

    while ((displays == -1) || (displays-- > 0))
    {
	/* get the current stats */
	get_system_info(&system_info);

	/* get the current set of processes */
	processes =
		get_process_info(&system_info,
				 &ps,
#ifdef ORDER
				 proc_compares[order_index]);
#else
				 proc_compare);
#endif

	/* display the load averages */
	(*d_loadave)(system_info.last_pid,
		     system_info.load_avg);

	/* display the current time */
	/* this method of getting the time SHOULD be fairly portable */
	time(&curr_time);
	i_timeofday(&curr_time);

	/* display process state breakdown */
	(*d_procstates)(system_info.p_total,
			system_info.procstates);

	/* display the cpu state percentage breakdown */
	(*d_cpustates)(system_info.cpustates);
	d_cpustates = u_cpustates;

	/* display memory stats */
	(*d_memory)(system_info.memory);

	/* handle message area */
	(*d_message)();

	/* update the header area */
	(*d_header)(header_text);
    
	if (topn > 0)
	{
	    /* determine number of processes to actually display */
	    /* this number will be the smallest of:  active processes,
	       number user requested, number current screen accomodates */
	    active_procs = system_info.p_active;
	    if (active_procs > topn)
	    {
		active_procs = topn;
	    }
	    if (active_procs > max_topn)
	    {
		active_procs = max_topn;
	    }

	    /* now show the top "n" processes. */
	    for (i = 0; i < active_procs; i++)
	    {
		(*d_process)(i, format_next_process(processes, get_userid));
	    }
	}
	else
	{
	    i = 0;
	}

	/* do end-screen processing */
	u_endscreen(i);

	/* now, flush the output buffer */
	fflush(stdout);

	/* only do the rest if we have more displays to show */
	if (displays)
	{
	    /* switch out for new display on smart terminals */
	    if (smart_terminal)
	    {
		if (overstrike)
		{
		    reset_display();
		}
		else
		{
		    d_loadave = u_loadave;
		    d_procstates = u_procstates;
		    d_cpustates = u_cpustates;
		    d_memory = u_memory;
		    d_message = u_message;
		    d_header = u_header;
		    d_process = u_process;
		}
	    }
    

	    no_command = Yes;
	    if (!interactive)
	    {
		/* set up alarm */
		(void) signal(SIGALRM, onalrm);
		(void) alarm((unsigned)delay);
    
		/* wait for the rest of it .... */
		pause();
	    }
	    else while (no_command)
	    {
		/* assume valid command unless told otherwise */
		no_command = No;

		/* set up arguments for select with timeout */
		FD_ZERO(&readfds);
		FD_SET(1, &readfds);		/* for standard input */
		timeout.tv_sec  = delay;
		timeout.tv_usec = 0;

		/* wait for either input or the end of the delay period */
		if (select(32, &readfds, (fd_set *)NULL, (fd_set *)NULL, &timeout) > 0)
		{
		    int newval;
		    char *errmsg;
    
		    /* something to read -- clear the message area first */
		    clear_message();

		    /* now read it and convert to command strchr */
		    /* (use "change" as a temporary to hold strchr) */
		    (void) read(0, &ch, 1);
		    if ((iptr = strchr(command_chars, ch)) == NULL)
		    {
			/* illegal command */
			new_message(MT_standout, 
				CATGETS(catd, _MSG_COMMAND_NOT_UNDERSTOOD));
			putchar('\r');
			no_command = Yes;
		    }
		    else
		    {
			change = iptr - command_chars;
			if (overstrike && change > CMD_OSLIMIT)
			{
			    /* error */
			    new_message(MT_standout,
				CATGETS(catd, _MSG_COMMAND_NOT_HANDLED));
			    putchar('\r');
			    no_command = Yes;
			}
			else switch(change)
			{
			    case CMD_redraw:	/* redraw screen */
				reset_display();
				break;
    
			    case CMD_update:	/* merely update display */
			    case CMD_noop:
				/* is the load average high? */
				if (system_info.load_avg[0] > LoadMax)
				{
				    /* yes, go home for visual feedback */
				    go_home();
				    fflush(stdout);
				}
				break;
	    
			    case CMD_quit:	/* quit */
				quit(0);
				/*NOTREACHED*/
				break;
	    
			    case CMD_help1:	/* help */
			    case CMD_help2:
				reset_display();
				clear();
				show_help();
				standout(CATGETS(catd, _MSG_HIT_KEY_CONTINUE));
				fflush(stdout);
				(void) read(0, &ch, 1);
				break;
	
			    case CMD_errors:	/* show errors */
				if (error_count() == 0)
				{
				    new_message(MT_standout,
					CATGETS(catd, _MSG_NO_ERROR_REPORT));
				    putchar('\r');
				    no_command = Yes;
				}
				else
				{
				    reset_display();
				    clear();
				    show_errors();
				    standout(CATGETS(catd, _MSG_HIT_KEY_CONTINUE));
				    fflush(stdout);
				    (void) read(0, &ch, 1);
				}
				break;
	
			    case CMD_number1:	/* new number */
			    case CMD_number2:
				new_message(MT_standout,
					CATGETS(catd, _MSG_NUM_PROCESS_SHOW));
				newval = readline(tempbuf1, 8, Yes);
				if (newval > -1)
				{
				    if (newval > max_topn)
				    {
					new_message(MT_standout | MT_delayed,
				CATGETS(catd, _MSG_DISPLAY_N_PROCESSES),
					  max_topn);
					putchar('\r');
				    }

				    if (newval == 0)
				    {
					/* inhibit the header */
					display_header(No);
				    }
				    else if (newval > topn && topn == 0)
				    {
					/* redraw the header */
					display_header(Yes);
					d_header = i_header;
				    }
				    topn = newval;
				}
				break;
	    
			    case CMD_delay:	/* new seconds delay */
				new_message(MT_standout, 
				CATGETS(catd, _MSG_SECONDS_DELAY));
				if ((i = readline(tempbuf1, 8, Yes)) > -1)
				{
				    delay = i;
				}
				clear_message();
				break;
	
			    case CMD_displays:	/* change display count */
				new_message(MT_standout,
					CATGETS(catd, _MSG_DISPLAYS_TO_SHOW),
					displays == -1 ? "infinite" :
							 itoa(displays));
				if ((i = readline(tempbuf1, 10, Yes)) > 0)
				{
				    displays = i;
				}
				else if (i == 0)
				{
				    quit(0);
				}
				clear_message();
				break;
    
			    case CMD_kill:	/* kill program */
				new_message(0, "kill ");
				if (readline(tempbuf2, sizeof(tempbuf2), No) > 0)
				{
				    if ((errmsg = kill_procs(tempbuf2)) != NULL)
				    {
					new_message(MT_standout, errmsg);
					putchar('\r');
					no_command = Yes;
				    }
				}
				else
				{
				    clear_message();
				}
				break;
	    
			    case CMD_renice:	/* renice program */
				new_message(0, "renice ");
				if (readline(tempbuf2, sizeof(tempbuf2), No) > 0)
				{
				    if ((errmsg = renice_procs(tempbuf2)) != NULL)
				    {
					new_message(MT_standout, errmsg);
					putchar('\r');
					no_command = Yes;
				    }
				}
				else
				{
				    clear_message();
				}
				break;

			    case CMD_idletog:
			    case CMD_idletog2:
				ps.idle = !ps.idle;
				if (ps.idle)
				 new_message(MT_standout | MT_delayed,
				  CATGETS(catd, _MSG_TOP_DISPLAYING_IDLE_PROCESSES));
				else
				 new_message(MT_standout | MT_delayed,
				  CATGETS(catd, _MSG_TOP_NOT_DISPLAYING_IDLE_PROCESSES));
				putchar('\r');
				break;

			    case CMD_systemtog:
				ps.system = !ps.system;
				if (ps.system)
				 new_message(MT_standout | MT_delayed,
				  CATGETS(catd, _MSG_TOP_DISPLAYING_SYS_PROCESSES));
				else
				 new_message(MT_standout | MT_delayed,
				  CATGETS(catd, _MSG_TOP_NOT_DISPLAYING_SYS_PROCESSES));
				putchar('\r');
				break;

			    case CMD_user:
				new_message(MT_standout,
				 CATGETS(catd, _MSG_USERNAME_SHOW));
				if (readline(tempbuf2, sizeof(tempbuf2), No) > 0)
				{
				    if (tempbuf2[0] == '+' &&
					tempbuf2[1] == '\0')
				    {
					ps.uid = -1;
				    }
				    else if ((i = userid(tempbuf2)) == -1)
				    {
					new_message(MT_standout,
					    CATGETS(catd, _MSG_UNKNOWN_USER),
						 tempbuf2);
					no_command = Yes;
				    }
				    else
				    {
					ps.uid = i;
				    }
				    putchar('\r');
				}
				else
				{
				    clear_message();
				}
				break;
	    
#ifdef ORDER
			    case CMD_order:
				new_message(MT_standout,
				    CATGETS(catd, _MSG_ORDER_SORT));
				if (readline(tempbuf2, sizeof(tempbuf2), No) > 0)
				{
				  if ((i = string_index(tempbuf2, statics.order_names)) == -1) {
					new_message(MT_standout,
					CATGETS(catd, _MSG_UNRECOGNIZED_SORT_ORDER), tempbuf2);
					no_command = Yes;
				    }
				    else
				    {
					order_index = i;
				    }
				    putchar('\r');
				}
				else
				{
				    clear_message();
				}
				break;
#endif
	    
			    default:
				new_message(MT_standout, CATGETS(catd, _MSG_BAD_CASE));
				putchar('\r');
			}
		    }

		    /* flush out stuff that may have been written */
		    fflush(stdout);
		}
	    }
	}
    }

    quit(0);
    /*NOTREACHED*/
}

/*
 *  reset_display() - reset all the display routine pointers so that entire
 *	screen will get redrawn.
 */

void reset_display()

{
    d_loadave    = i_loadave;
    d_procstates = i_procstates;
    d_cpustates  = i_cpustates;
    d_memory     = i_memory;
    d_message	 = i_message;
    d_header	 = i_header;
    d_process	 = i_process;
}

/*
 *  signal handlers
 */

sigret_t leave()	/* exit under normal conditions -- INT handler */

{
    end_screen();
    exit(0);
}

sigret_t tstop()	/* SIGTSTP handler */

{
    /* move to the lower left */
    end_screen();
    fflush(stdout);

    /* default the signal handler action */
    (void) signal(SIGTSTP, SIG_DFL);

    /* unblock the signal and send ourselves one */
#ifdef SIGRELSE
    sigrelse(SIGTSTP);
#else
    (void) sigsetmask(sigblock(0) & ~(1 << (SIGTSTP - 1)));
#endif
    (void) kill(0, SIGTSTP);

    /* reset the signal handler */
    (void) signal(SIGTSTP, tstop);

    /* reinit screen */
    reinit_screen();

    /* jump to appropriate place */
    longjmp(jmp_int, 1);

    /*NOTREACHED*/
}

#ifdef SIGWINCH
sigret_t mywinch()		/* SIGWINCH handler */

{
    /* reascertain the screen dimensions */
    get_screensize();

    /* tell display to resize */
    max_topn = display_resize();

    /* reset the signal handler */
    (void) signal(SIGWINCH, mywinch);

    /* jump to appropriate place */
    longjmp(jmp_int, 1);
}
#endif

void quit(status)		/* exit under duress */

int status;

{
    end_screen();
    exit(status);
    /*NOTREACHED*/
}

sigret_t onalrm()	/* SIGALRM handler */

{
    /* this is only used in batch mode to break out of the pause() */
    /* return; */
}

