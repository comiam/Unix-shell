#include "signals.h"

void set_signal_handler(int sig, void *handler)
{
    struct sigaction act;

    act.sa_handler = handler;
    act.sa_flags = 0;

    if(sig == SIGCHLD)
        act.sa_flags |= SA_RESTART;

    sigemptyset(&act.sa_mask);
    sigaction(sig, &act, ((void *)0));
}