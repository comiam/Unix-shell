#ifndef UNIX_SHELL_SIGNALS_H
#define UNIX_SHELL_SIGNALS_H

#include <signal.h>

/* Set signal handler for special signal. */
void set_signal_handler(int sig, void *handler);

#endif
