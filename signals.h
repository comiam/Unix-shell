#ifndef UNIX_SHELL_SIGNALS_H
#define UNIX_SHELL_SIGNALS_H

#include <signal.h>

void set_signal_handler(int sig, void *handler);

#endif
