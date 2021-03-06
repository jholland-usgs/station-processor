#pragma ident "$Id: ttytst2.c,v 1.1 2005/05/26 23:52:35 dechavez Exp $"
/*======================================================================
 *
 *  Test serial lines by writing/reading simple byte stream.
 *
 *  The program loops forever, until SIGINT or SIGTERM is received.
 *
 *  A status report is logged upon receipt of SIGHUP.
 *
 *====================================================================*/
#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>
#include "util.h"
#include "oldttyio.h"

#define DEF_BAUD  9600
#define DEF_TO      10
#define DEF_BUF      1
#define DEF_FLOW  TTYIO_FSOFT

struct termios old_termios, new_termios;
int  fd;
int action     = 0;
char *port     = NULL;
int  pid;

long count, errors, nlost;

struct flagmap {
    unsigned long val; 
    char *txt;
};

void help(myname)
char *myname;
{
    fprintf(stderr, "\n");
    fprintf(stderr, "to read:  %s +r device [options]\n", myname);
    fprintf(stderr, "to write: %s +w device [options]\n", myname);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "baud=speed   - baud rate\n");
    fprintf(stderr, "to=secs      - serial port timeout\n");
    fprintf(stderr, "flow=hard    - hardware flow control\n");
    fprintf(stderr, "flow=soft    - xon/xoff flow control\n");
    fprintf(stderr, "flow=none    - no flow control\n");
    fprintf(stderr, "log=name     - logfile name\n");
    fprintf(stderr, "debug=val    - initial debug level\n");
    fprintf(stderr, "buf=len      - buffer length\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "defaults: ");
    fprintf(stderr, "baud=%d ", DEF_BAUD);
    fprintf(stderr, "to=%d ",   DEF_TO);
    fprintf(stderr, "buf=%d ",  DEF_BUF);
    fprintf(stderr, "flow=soft ");
    fprintf(stderr, "\n");
    exit(1);
}

void log_stats()
{
    if (action == TTYIO_WRITE) {
        util_log(1, "%7ld bytes sent", count);
    } else {
        util_log(1, "%7ld bytes received", count);
        util_log(1, "%7ld errors", errors);
        util_log(1, "%7ld bytes lost", nlost);
        util_log(1, "%7ld total bytes sent?", count + nlost);
    }
}

static void catcher(int sig)
{
    util_log(1, "%s", util_sigtoa(sig));
    
    if (sig == SIGHUP) {
        ttyio_loginfo(1, port, &new_termios);
        log_stats();
    } else {
        log_stats();

    /*  Close files  */

        close(fd);
        if (sig == SIGINT || sig == SIGTERM) exit(0);
        exit(1);
    }
}

main(argc, argv)
int argc;
char *argv[];
{
unsigned long speed = 0;
char input, *test;
caddr_t arg;
long nread, len = DEF_BUF;
int  i, status, got, lost, quit;
int  debug  = 1;
int  flow   = DEF_FLOW;
int  to     = DEF_TO;
int  baud   = DEF_BAUD;
unsigned char value, prev_byte, expected, *buffer;
char *log     = NULL;

/*  Scan command line for default overrides  */

    pid = getpid();

    for (i = 1; i < argc; i++) {

        if (strcmp(argv[i], "+r") == 0) {
            action = TTYIO_READ;

        } else if (strcmp(argv[i], "+w") == 0) {
            action = TTYIO_WRITE;

        } else if (strncmp(argv[i], "flow=", strlen("flow=")) == 0) {
            arg = argv[i]+strlen("flow=");
            if (strcmp(arg, "soft") == 0) {
                flow = TTYIO_FSOFT;
            } else if (strcmp(arg, "hard") == 0) {
                flow = TTYIO_FHARD;
            } else if (strcmp(arg, "none") == 0) {
                flow = TTYIO_FNONE;
            } else {
                fprintf(stderr, "illegal flow argument `%s'\n", arg);
                help(argv[0]);
            }

        } else if (strncmp(argv[i], "log=", strlen("log=")) == 0) {
            log = argv[i] + strlen("log=");

        } else if (strncmp(argv[i], "baud=", strlen("baud=")) == 0) {
            baud = (speed_t) atol(argv[i]+strlen("baud="));

        } else if (strncmp(argv[i], "buf=", strlen("buf=")) == 0) {
            arg = argv[i]+strlen("buf=");
            if ((len = atol(arg)) <= 0) {
                fprintf(stderr, "illegal buf argument `%s'\n", arg);
                help(argv[0]);
            }

        } else if (strncmp(argv[i], "debug=", strlen("debug=")) == 0) {
            debug = atoi(argv[i]+strlen("debug="));

        } else if (strncmp(argv[i], "to=", strlen("to=")) == 0) {
            to = atoi(argv[i]+strlen("to="));

        } else {
            port = argv[i];
        }
    }

    if (action == 0 || port == NULL) help(argv[0]);

/*  Prepare buffer  */

    if ((buffer = (unsigned char *) malloc(len)) == NULL) {
        fprintf(stderr,"ttytst2[%d]: ", pid);
        perror("malloc");
        exit(1);
    }

/*  Open port  */

    fd = ttyio_init(port,action,baud,baud,TTYIO_PNONE,flow,&old_termios,&new_termios,1);
    if (fd < 0) {
        fprintf(stderr,"ttytst2[%d]: ttyio_init error %d: ", pid, -fd);
        perror(port);
        exit(1);
    }

/*  Set up exit handlers  */

    signal(SIGHUP,  catcher);
    signal(SIGINT,  catcher);
    signal(SIGTERM, catcher);
    signal(SIGQUIT, catcher);
    signal(SIGUSR1, catcher);
    signal(SIGUSR2, catcher);

/* Start logging */

    util_logopen(log, 1, 9, debug, NULL, argv[0]);
    util_log(1, "current port parameters");
    ttyio_loginfo(1, port, &new_termios);

/*  Read or write, as appropriate */

    count = 0;
    value = 0;
    quit  = 0;

    if (action == TTYIO_READ) {

        util_log(1, "begining read loop");

    /* Loop forever, reading from tty */

        while (1) {

            if ((nread = util_read(fd, (void *) buffer, (int) len, to)) != len) {
                if (errno == ETIMEDOUT) {
                    quit = 1;
                    util_log(1, "timed out, nread = %d", nread);
                } else {
                    util_log(1, "util_read: %s", syserrmsg(errno));
                    log_stats();
                    exit(1);
                }
            }

            for (i = 0; i < nread; i++) {
                value = buffer[i];
                if (count > 0) {
                    expected = prev_byte + 1;
                    if (value != expected) {
                        got = (int) value;
                        if (value < prev_byte) got += 256;
                        lost = got - (int) expected;
                        util_log(1, "expected 0x%02x, got 0x%02x",
                            expected, value
                        );
                        util_log(1, "%d bytes dropped", lost);
                        nlost += (long) lost;
                        ++errors;
                    }
                } else {
                    util_log(1, "initial byte received, value = 0x%02x", value);
                }

                prev_byte = value;
                ++count;

                if (count % 102400 == 0) {
                    util_log(2, "%ld bytes received so far", count);
                }
            }
            if (quit) {
                log_stats();
                exit(0);
            }
        }

    } else {

    /* Loop forever, writing single bytes that increment by 1 */

        util_log(1, "begining write loop");

        while (1) {

            for (i = 0; i < len; i++) buffer[i] = value++;

            if (util_write(fd, buffer, len, to) != len) {
                util_log(1, "util_write: %s", syserrmsg(errno));
                log_stats();
                exit(1);
            }

            count += len;

            if (count % 102400 == 0) {
                util_log(2, "%ld bytes sent so far", count);
            }
        }
    }
}

/* Revision History
 *
 * $Log: ttytst2.c,v $
 * Revision 1.1  2005/05/26 23:52:35  dechavez
 * brought over from original ttyutil
 *
 */
