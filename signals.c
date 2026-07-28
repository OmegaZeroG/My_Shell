#include "my_shell.h"

/*
 * Signal architecture:
 *
 *   Parent (shell loop)
 *     SIGINT  -> SIG_IGN   shell itself must survive Ctrl+C
 *     SIGTSTP -> SIG_IGN   shell itself must survive Ctrl+Z
 *     SIGCHLD -> handler   just flags that a child changed state; the actual
 *                          waitpid(WNOHANG) reaping happens in the main loop
 *                          (see reap_background_jobs in jobs.c), never
 *                          inside the handler itself, because printf/malloc
 *                          are not async-signal-safe.
 *
 *   Child (right after fork, before execve/child_process)
 *     SIGINT, SIGTSTP, SIGCHLD -> SIG_DFL  so a foreground child can be
 *     killed/suspended normally and can itself waitpid its own children.
 */

volatile sig_atomic_t g_sigchld_received = 0;

static void sigchld_handler(int sig) {
    (void)sig;
    g_sigchld_received = 1;
}

void setup_parent_signals(void) {
    struct sigaction sa_ignore;
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;
    sigaction(SIGINT, &sa_ignore, NULL);
    sigaction(SIGTSTP, &sa_ignore, NULL);

    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa_chld, NULL);
}

void reset_child_signals(void) {
    struct sigaction sa_default;
    sa_default.sa_handler = SIG_DFL;
    sigemptyset(&sa_default.sa_mask);
    sa_default.sa_flags = 0;
    sigaction(SIGINT, &sa_default, NULL);
    sigaction(SIGTSTP, &sa_default, NULL);
    sigaction(SIGCHLD, &sa_default, NULL);
}
