#include "my_shell.h"

/*
 * Minimal background-job bookkeeping: `cmd &` returns to the prompt
 * immediately, and we print a "Done" notification the next time the shell
 * loop turns over after the child actually exits. There's no fg/bg/jobs
 * builtin here (that needs terminal process-group control via tcsetpgrp) -
 * this is intentionally the reap-and-notify half of job control, not the
 * interactive half.
 */

typedef struct {
    pid_t pid;
    char command[MAX_CMD_LEN];
} BgJob;

static BgJob g_jobs[MAX_JOBS];
static int g_job_count = 0;

void job_add(pid_t pid, const char* cmd) {
    if (g_job_count >= MAX_JOBS) {
        fprintf(stderr, "my_shell: too many background jobs, not tracking pid %d\n", pid);
        return;
    }
    g_jobs[g_job_count].pid = pid;
    my_strcpy(g_jobs[g_job_count].command, cmd, MAX_CMD_LEN);
    g_job_count++;
}

// Called once per shell-loop iteration. Cheap no-op unless SIGCHLD actually
// fired since the last call.
void reap_background_jobs(void) {
    if (!g_sigchld_received) return;
    g_sigchld_received = 0;

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < g_job_count; i++) {
            if (g_jobs[i].pid == pid) {
                printf("[bg] pid %d done: %s\n", pid, g_jobs[i].command);
                g_jobs[i] = g_jobs[g_job_count - 1];
                g_job_count--;
                break;
            }
        }
    }
}
