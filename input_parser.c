#include "my_shell.h"

/*
 * Tokenizer + recursive-descent-ish parser that turns a raw input line into
 * a list of Pipeline structs. Grammar (informal):
 *
 *   line     := pipeline (connector pipeline)*
 *   pipeline := command ('|' command)* ['&']
 *   command  := (word | redirection)+
 *   connector:= '&&' | '||' | ';'
 *
 * Quoted substrings ("like this" or 'like this') are treated as part of a
 * single word and may be glued to unquoted text, e.g. foo"bar baz"qux.
 */

typedef enum {
    TOK_WORD,
    TOK_PIPE,
    TOK_REDIR_IN,
    TOK_REDIR_OUT,
    TOK_REDIR_APPEND,
    TOK_AND,
    TOK_OR,
    TOK_SEMI,
    TOK_BG
} TokType;

typedef struct {
    TokType type;
    char* value; // heap-allocated, only set for TOK_WORD
} Tok;

static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

static int is_operator_start(char c) {
    return c == '|' || c == '<' || c == '>' || c == ';' || c == '&';
}

// Reads one whitespace-delimited word, honouring quotes, starting at *pp.
static char* read_word(const char** pp) {
    const char* s = *pp;
    size_t cap = 32, len = 0;
    char* buf = malloc(cap);
    if (!buf) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    while (*s && !is_space(*s) && !is_operator_start(*s)) {
        if (*s == '"' || *s == '\'') {
            char quote = *s++;
            while (*s && *s != quote) {
                if (len + 1 >= cap) {
                    cap *= 2;
                    buf = realloc(buf, cap);
                }
                buf[len++] = *s++;
            }
            if (*s == quote) s++; // consume closing quote
        } else {
            if (len + 1 >= cap) {
                cap *= 2;
                buf = realloc(buf, cap);
            }
            buf[len++] = *s++;
        }
    }

    buf[len] = '\0';
    *pp = s;
    return buf;
}

static Tok* tokenize(const char* input, int* out_count) {
    size_t cap = 32, count = 0;
    Tok* toks = malloc(cap * sizeof(Tok));
    const char* s = input;

    while (*s) {
        while (is_space(*s)) s++;
        if (!*s) break;

        Tok t;
        if (s[0] == '&' && s[1] == '&')      { t.type = TOK_AND;          t.value = NULL; s += 2; }
        else if (s[0] == '|' && s[1] == '|') { t.type = TOK_OR;           t.value = NULL; s += 2; }
        else if (s[0] == '>' && s[1] == '>') { t.type = TOK_REDIR_APPEND; t.value = NULL; s += 2; }
        else if (s[0] == '|')                { t.type = TOK_PIPE;        t.value = NULL; s += 1; }
        else if (s[0] == '<')                { t.type = TOK_REDIR_IN;    t.value = NULL; s += 1; }
        else if (s[0] == '>')                { t.type = TOK_REDIR_OUT;   t.value = NULL; s += 1; }
        else if (s[0] == ';')                { t.type = TOK_SEMI;        t.value = NULL; s += 1; }
        else if (s[0] == '&')                { t.type = TOK_BG;          t.value = NULL; s += 1; }
        else                                  { t.type = TOK_WORD;        t.value = read_word(&s); }

        if (count + 1 >= cap) {
            cap *= 2;
            toks = realloc(toks, cap * sizeof(Tok));
        }
        toks[count++] = t;
    }

    *out_count = (int)count;
    return toks;
}

static Command* new_command(void) {
    Command* c = malloc(sizeof(Command));
    if (!c) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    c->argv = malloc(sizeof(char*) * (MAX_ARGS + 1));
    c->argc = 0;
    c->infile = NULL;
    c->outfile = NULL;
    c->append = 0;
    c->argv[0] = NULL;
    return c;
}

static void command_add_arg(Command* c, char* word) {
    if (c->argc < MAX_ARGS) {
        c->argv[c->argc++] = word;
        c->argv[c->argc] = NULL;
    } else {
        fprintf(stderr, "my_shell: too many arguments, ignoring extra\n");
        free(word);
    }
}

void free_command(Command* cmd) {
    if (!cmd) return;
    for (int i = 0; i < cmd->argc; i++) free(cmd->argv[i]);
    free(cmd->argv);
    free(cmd->infile);
    free(cmd->outfile);
    free(cmd);
}

void free_pipeline(Pipeline* p) {
    if (!p) return;
    for (int i = 0; i < p->count; i++) free_command(p->cmds[i]);
    free(p->cmds);
    free(p);
}

void free_pipelines(Pipeline** pipelines, int count) {
    if (!pipelines) return;
    for (int i = 0; i < count; i++) free_pipeline(pipelines[i]);
    free(pipelines);
}

// Small growable arrays used only while parsing.
typedef struct { Command** items; size_t count, cap; } CmdList;
typedef struct { Pipeline** items; size_t count, cap; } PipeList;

static void cmdlist_push(CmdList* l, Command* c) {
    if (l->count + 1 >= l->cap) {
        l->cap = l->cap ? l->cap * 2 : 4;
        l->items = realloc(l->items, l->cap * sizeof(Command*));
    }
    l->items[l->count++] = c;
}

static void pipelist_push(PipeList* l, Pipeline* p) {
    if (l->count + 1 >= l->cap) {
        l->cap = l->cap ? l->cap * 2 : 4;
        l->items = realloc(l->items, l->cap * sizeof(Pipeline*));
    }
    l->items[l->count++] = p;
}

// Finalises the command being built (if non-empty) into cmds, wraps cmds
// into a Pipeline with the given connector/background flag, and resets
// cur/cmds for the next pipeline.
static void finish_pipeline(CmdList* cmds, Command** cur, PipeList* pipelines,
                             int connector, int background) {
    if ((*cur)->argc > 0) {
        cmdlist_push(cmds, *cur);
    } else {
        free_command(*cur);
    }

    Pipeline* p = malloc(sizeof(Pipeline));
    p->cmds = cmds->items;
    p->count = (int)cmds->count;
    p->connector = connector;
    p->background = background;
    pipelist_push(pipelines, p);

    cmds->items = NULL;
    cmds->count = 0;
    cmds->cap = 0;
    *cur = new_command();
}

Pipeline** parse_input(const char* input, int* out_count) {
    int ntoks;
    Tok* toks = tokenize(input, &ntoks);

    PipeList pipelines = {0};
    CmdList cmds = {0};
    Command* cur = new_command();
    int syntax_error = 0;

    for (int i = 0; i < ntoks && !syntax_error; i++) {
        Tok* t = &toks[i];

        switch (t->type) {
        case TOK_WORD:
            command_add_arg(cur, t->value);
            t->value = NULL; // ownership transferred to cur->argv
            break;

        case TOK_REDIR_IN:
        case TOK_REDIR_OUT:
        case TOK_REDIR_APPEND:
            if (i + 1 >= ntoks || toks[i + 1].type != TOK_WORD) {
                fprintf(stderr, "my_shell: syntax error: expected filename after redirection\n");
                syntax_error = 1;
                break;
            }
            i++;
            if (t->type == TOK_REDIR_IN) {
                free(cur->infile);
                cur->infile = toks[i].value;
            } else {
                free(cur->outfile);
                cur->outfile = toks[i].value;
                cur->append = (t->type == TOK_REDIR_APPEND);
            }
            toks[i].value = NULL; // ownership transferred to cur->infile/outfile
            break;

        case TOK_PIPE:
            if (cur->argc == 0) {
                fprintf(stderr, "my_shell: syntax error near '|'\n");
                syntax_error = 1;
                break;
            }
            cmdlist_push(&cmds, cur);
            cur = new_command();
            break;

        case TOK_BG:
            if (cur->argc == 0 && cmds.count == 0) {
                fprintf(stderr, "my_shell: syntax error near '&'\n");
                syntax_error = 1;
                break;
            }
            finish_pipeline(&cmds, &cur, &pipelines, 0, 1);
            break;

        case TOK_AND:
        case TOK_OR:
        case TOK_SEMI:
            if (cur->argc == 0 && cmds.count == 0) {
                fprintf(stderr, "my_shell: syntax error: empty command\n");
                syntax_error = 1;
                break;
            }
            finish_pipeline(&cmds, &cur, &pipelines,
                             t->type == TOK_AND ? 1 : t->type == TOK_OR ? 2 : 0,
                             0);
            break;
        }
    }

    // Every TOK_WORD's value is nulled out the moment it's handed off to a
    // Command (see the TOK_WORD/TOK_REDIR_* cases above), so this sweep only
    // ever frees words that a syntax error left stranded mid-tokenize —
    // it's a no-op double-free-safe cleanup either way.
    for (int i = 0; i < ntoks; i++) {
        if (toks[i].type == TOK_WORD) free(toks[i].value);
    }
    free(toks);

    // A trailing '|' (or "cmd1 | cmd2 |" etc.) leaves earlier pipe stages
    // already queued in cmds but the final stage empty — that's a dangling
    // pipe, not a valid 1-stage pipeline, so it's a syntax error too.
    if (!syntax_error && cmds.count > 0 && cur->argc == 0) {
        fprintf(stderr, "my_shell: syntax error: expected a command after '|'\n");
        syntax_error = 1;
    }

    if (syntax_error) {
        free_command(cur);
        for (size_t i = 0; i < cmds.count; i++) free_command(cmds.items[i]);
        free(cmds.items);
        for (size_t i = 0; i < pipelines.count; i++) free_pipeline(pipelines.items[i]);
        free(pipelines.items);
        *out_count = 0;
        return NULL;
    }

    // Flush whatever is left as the final pipeline (implicit terminator).
    // finish_pipeline() always leaves `cur` pointing at a freshly allocated,
    // still-empty Command for "the next one" — but there is no next one, so
    // free it right back.
    if (cur->argc > 0 || cmds.count > 0) {
        finish_pipeline(&cmds, &cur, &pipelines, 0, 0);
        free_command(cur);
    } else {
        free_command(cur);
    }

    if (pipelines.count == 0) {
        *out_count = 0;
        return NULL;
    }

    *out_count = (int)pipelines.count;
    return pipelines.items;
}

// ---- Variable expansion ($VAR, $?) ---------------------------------------

static char* expand_word(const char* word, char** env, int last_status) {
    if (word[0] != '$' || word[1] == '\0') return my_strdup(word);

    const char* name = word + 1;
    if (my_strcmp(name, "?") == 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", last_status);
        return my_strdup(buf);
    }

    char* val = my_getenv(name, env);
    return my_strdup(val ? val : "");
}

void expand_pipeline(Pipeline* p, char** env, int last_status) {
    for (int i = 0; i < p->count; i++) {
        Command* c = p->cmds[i];

        for (int j = 0; j < c->argc; j++) {
            if (c->argv[j][0] == '$' && c->argv[j][1] != '\0') {
                char* expanded = expand_word(c->argv[j], env, last_status);
                free(c->argv[j]);
                c->argv[j] = expanded;
            }
        }
        if (c->infile && c->infile[0] == '$' && c->infile[1] != '\0') {
            char* e = expand_word(c->infile, env, last_status);
            free(c->infile);
            c->infile = e;
        }
        if (c->outfile && c->outfile[0] == '$' && c->outfile[1] != '\0') {
            char* e = expand_word(c->outfile, env, last_status);
            free(c->outfile);
            c->outfile = e;
        }
    }
}
