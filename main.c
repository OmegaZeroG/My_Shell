#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "my_shell.h"

// Non-interactive fallback (piped/redirected stdin): plain line reading, no
// raw-mode terminal handling. This is what lets the shell be scripted, e.g.
//   ./my_shell < tests/commands.txt
//   echo "pwd" | ./my_shell
static char* read_line_plain(void) {
    char* line = NULL;
    size_t cap = 0;
    ssize_t len = getline(&line, &cap, stdin);
    if (len == -1) {
        free(line);
        return NULL; // EOF
    }
    if (len > 0 && line[len - 1] == '\n')
        line[len - 1] = '\0';
    return line;
}

static void shell_loop(char** env) {
    int env_is_heap = 0;
    int last_status = 0;
    char* initial_directory = getcwd(NULL, 0);
    int interactive = isatty(STDIN_FILENO);

    History hist;
    history_init(&hist);
    setup_parent_signals();

    while (1) {
        reap_background_jobs();

        if (interactive) {
            printf("[my_shell]$ ");
            fflush(stdout);
        }

        char* input = interactive ? read_input(&hist) : read_line_plain();

        if (!input) {
            if (interactive) printf("\n");
            break; // EOF (Ctrl+D, or end of piped input)
        }

        if (input[0] == '\0') {
            free(input);
            continue;
        }

        if (interactive)
            history_add(&hist, input);

        int pipeline_count = 0;
        Pipeline** pipelines = parse_input(input, &pipeline_count);
        free(input);

        if (!pipelines)
            continue; // empty input or a syntax error already reported

        int skip = 0;
        for (int i = 0; i < pipeline_count; i++) {
            Pipeline* p = pipelines[i];

            if (!skip) {
                expand_pipeline(p, env, last_status);
                last_status = run_pipeline(p, &env, &env_is_heap, initial_directory);
            }

            if (p->connector == 1)      skip = (last_status != 0); // &&
            else if (p->connector == 2) skip = (last_status == 0); // ||
            else                         skip = 0;                  // ';' or end of line
        }

        free_pipelines(pipelines, pipeline_count);
    }

    history_free(&hist);
    free(initial_directory);
    if (env_is_heap) {
        for (int i = 0; env[i]; i++)
            free(env[i]);
        free(env);
    }
}

int main(int argc, char** argv, char** env) {
    (void)argc;
    (void)argv;

    
    setvbuf(stdout, NULL, _IOLBF, 0);

    shell_loop(env);

    return 0;
}
