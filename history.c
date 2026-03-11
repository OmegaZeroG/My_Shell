#include "my_shell.h"

// Initialize history struct
void history_init(History* hist) {
    hist->count = 0;
    hist->current = 0;
    for (int i = 0; i < HISTORY_SIZE; i++)
        hist->commands[i] = NULL;
}

// Add a command to history
void history_add(History* hist, char* cmd) {
    if (!cmd || cmd[0] == '\0')
        return;

    // Don't add duplicate of last command
    if (hist->count > 0 && my_strcmp(hist->commands[hist->count - 1], cmd) == 0)
        return;

    // If history is full, free oldest and shift everything down
    if (hist->count == HISTORY_SIZE) {
        free(hist->commands[0]);
        for (int i = 0; i < HISTORY_SIZE - 1; i++)
            hist->commands[i] = hist->commands[i + 1];
        hist->count--;
    }

    hist->commands[hist->count++] = my_strdup(cmd);
    hist->current = hist->count; // reset navigation to end
}

// Free all history
void history_free(History* hist) {
    for (int i = 0; i < hist->count; i++) {
        free(hist->commands[i]);
        hist->commands[i] = NULL;
    }
    hist->count = 0;
    hist->current = 0;
}

// Enable raw mode — read char by char, no echo
void enable_raw_mode(struct termios* orig) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, orig);
    raw = *orig;
    raw.c_lflag &= ~(ECHO | ICANON); // disable echo and line buffering
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Restore original terminal settings
void disable_raw_mode(struct termios* orig) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig);
}

// Clear current line on terminal and reprint prompt + buffer
static void refresh_line(const char* buf, int pos) {
    // Move to start of line, clear it, reprint
    write(STDOUT_FILENO, "\r", 1);
    write(STDOUT_FILENO, "\033[2K", 4);         // clear entire line
    write(STDOUT_FILENO, "[my_shell]$ ", 12);   // reprint prompt
    write(STDOUT_FILENO, buf, pos);              // reprint current buffer
}

// Main input reader — handles arrow keys, backspace, normal chars
char* read_input(History* hist) {
    struct termios orig;
    enable_raw_mode(&orig);

    char buf[MAX_INPUT];
    int pos = 0;
    buf[0] = '\0';

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            // EOF (Ctrl+D)
            disable_raw_mode(&orig);
            if (pos == 0) {
                write(STDOUT_FILENO, "\n", 1);
                return NULL; // signal EOF to shell_loop
            }
            break;
        }

        if (c == '\n' || c == '\r') {
            // Enter pressed
            write(STDOUT_FILENO, "\n", 1);
            break;

        } else if (c == 127 || c == '\b') {
            // Backspace
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                refresh_line(buf, pos);
            }

        } else if (c == '\033') {
            // Escape sequence — read next 2 bytes
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0) continue;
            if (read(STDIN_FILENO, &seq[1], 1) <= 0) continue;

            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    // ↑ Up arrow — go to previous command
                    if (hist->current > 0) {
                        hist->current--;
                        pos = my_strlen(hist->commands[hist->current]);
                        my_strcpy(buf, hist->commands[hist->current], MAX_INPUT);
                        refresh_line(buf, pos);
                    }
                } else if (seq[1] == 'B') {
                    // ↓ Down arrow — go to next command
                    if (hist->current < hist->count - 1) {
                        hist->current++;
                        pos = my_strlen(hist->commands[hist->current]);
                        my_strcpy(buf, hist->commands[hist->current], MAX_INPUT);
                        refresh_line(buf, pos);
                    } else {
                        // Past the end — clear line
                        hist->current = hist->count;
                        pos = 0;
                        buf[0] = '\0';
                        refresh_line(buf, pos);
                    }
                }
                // ← → (left/right) — ignore for now
            }

        } else if (c == '\003') {
            // Ctrl+C — clear line
            pos = 0;
            buf[0] = '\0';
            write(STDOUT_FILENO, "^C\n", 3);
            disable_raw_mode(&orig);
            return my_strdup(""); // return empty string to continue loop

        } else if (pos < MAX_INPUT - 1) {
            // Normal printable character
            buf[pos++] = c;
            buf[pos] = '\0';
            write(STDOUT_FILENO, &c, 1); // echo the character
        }
    }

    disable_raw_mode(&orig);
    buf[pos] = '\0';
    return my_strdup(buf);
}
