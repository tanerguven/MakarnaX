#ifndef _SIGNAL_H
#define _SIGNAL_H

/** Hangup */
#define SIGHUP 1
/** Interrupt */
#define SIGINT 2
/** Quit and dump core */
#define SIGQUIT 3
/** Illegal instruction */
#define SIGILL 4
/** Trace/breakpoint trap */
#define SIGTRAP 5
/** Process aborted */
#define SIGABRT 6
/** Bus error: "access to undefined portion of memory object" */
#define SIGBUS 7
/** Floating point exception: "erroneous arithmetic operation" */
#define SIGFPE 8
/** Kill (terminate immediately) */
#define SIGKILL 9
/** User-defined 1 */
#define SIGUSR1 10
/** Segmentation violation */
#define SIGSEGV 11
/** User-defined 2 */
#define SIGUSR2 12
/** Write to pipe with no one reading */
#define SIGPIPE 13
/** Signal raised by alarm */
#define SIGALRM 14
/** Termination (request to terminate) */
#define SIGTERM 15
/** Child process terminated, stopped (or continued*) */
#define SIGCHLD 17
/** Continue if stopped */
#define SIGCONT 18
/** Stop executing temporarily */
#define SIGSTOP 19
/** Terminal stop signal */
#define SIGTSTP 20
/** Background process attempting to read from tty ("in") */
#define SIGTTIN 21
/** Background process attempting to write to tty ("out") */
#define SIGTTOU 22
/** Urgent data available on socket */
#define SIGURG 23
/** CPU time limit exceeded */
#define SIGXCPU 24
/** File size limit exceeded */
#define SIGXFSZ 25
/** Signal raised by timer counting virtual time: "virtual timer expired" */
#define SIGVTALARM 26
/** Profiling timer expired */
#define SIGPROF 27
/** Pollable event */
#define SIGPOLL 29
/** Bad syscall */
#define SIGSYS 31

#endif /* _SIGNAL_H */
