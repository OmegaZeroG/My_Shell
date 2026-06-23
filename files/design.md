# design.md — My_Shell
**Design Decisions & Rationale**
*Every decision here is something an interviewer may ask about.*

---

## 1. Why `fork` + `execve` instead of `system()`?

`system("ls")` works but hides everything. Under the hood it calls `/bin/sh -c "ls"` — you're spawning a second shell. You can't control the child's environment, can't do custom fd manipulation before exec, can't implement pipes or redirection.

`fork` + `execve` gives full control:
- `fork` creates a child with a copy of the parent's address space (copy-on-write)
- Between `fork` and `execve` you can: set up pipes, redirect fds, reset signals, set process group
- `execve` replaces the child's process image entirely — only the open file descriptors survive

**Interview question this answers:** "Why is `cd` a builtin?" — Because `cd` calls `chdir()`. If it ran in a child process, `chdir()` changes the child's working directory. When the child exits, the parent's cwd is unchanged. `cd` must run in the parent (the shell itself) to affect the shell's directory.

---

## 2. Why `execve` and not `execvp`?

`execvp` does PATH resolution internally. Using `execve` means PATH resolution is done manually — this is intentional. It demonstrates understanding of how PATH works: iterate directories, build `path + "/" + cmd`, call `access(X_OK)` or just try `execve` and let it fail. This is exactly what the kernel would do.

Using `execvp` is easier but there's nothing to explain in an interview.

---

## 3. Why a custom string library (no `<string.h>`)?

This was a deliberate constraint, not a requirement. Benefits:
- Forces understanding of how string operations actually work at memory level
- Interviewers ask: "Walk me through your `my_strdup`" — easy to answer
- Shows you can operate without abstractions

The cost: more code to maintain, more potential bugs. Trade-off accepted for learning value.

**Important to know:** in real production C code you'd always use `<string.h>`. Custom string libs exist in constrained embedded environments (no libc) or educational contexts. Be honest about this in interviews.

---

## 4. Why `char**` for environment, not a hash map?

The Unix convention: `execve` takes `char** envp`, the third argument. The kernel expects exactly this format — a NULL-terminated array of `"KEY=VALUE"` strings. If we stored env as a hash map, we'd need to convert it back to `char**` on every `execve` call.

Keeping it as `char**` means:
- Zero conversion overhead
- Direct pass to `execve`
- Matches what `main(int argc, char** argv, char** envp)` gives us

Cost: O(n) lookup for `getenv`, O(n) for setenv (scan for existing key). Acceptable — PATH typically has < 100 entries.

---

## 5. Why track `env_is_heap`?

The initial `env` pointer comes from `main`'s third argument — this memory is managed by the OS, not by us. We must never `free()` it.

After the first `setenv`/`unsetenv`, we `malloc` a new array. Now we own it and must free it when done or when replacing.

`env_is_heap = 0` initially. Set to `1` after first `setenv`/`unsetenv`. Before replacing `env`, check flag and free if set.

**Alternative:** duplicate env at startup into heap. Simpler flag logic, slightly more startup memory. Not chosen — unnecessary allocation if user never calls setenv.

---

## 6. Why close unused pipe ends? (Critical)

```
pipe() → [read_end=3, write_end=4]
```

After `fork`, both parent and both children have both ends open. If child2 (the reader) is waiting for EOF from the pipe, it will block until ALL write ends are closed. If parent still holds `write_end=4` open, child2 never sees EOF — it blocks forever. Deadlock.

Rule: every process that doesn't use an fd must close it immediately after fork, before exec.

**This is the single most commonly asked pipe implementation question.** Know it cold.

---

## 7. Why `setpgid(0, 0)` for background jobs?

When you press Ctrl+C, the terminal sends SIGINT to the **foreground process group**. If a background job is in the same process group as the shell, it would receive SIGINT too — wrong.

`setpgid(0, 0)` puts the child in its own process group (pgid = child's pid). Now Ctrl+C only affects the foreground group. Background jobs are immune.

Same principle for `fg`: to give a background job keyboard input, you must make it the foreground process group with `tcsetpgrp(STDIN_FILENO, job_pgid)`. When it finishes, restore the shell's pgid.

---

## 8. Why `waitpid(WNOHANG)` for SIGCHLD?

`waitpid(-1, &status, 0)` blocks until any child exits. In a SIGCHLD handler, we can't block — the shell must keep running.

`waitpid(-1, &status, WNOHANG)` returns immediately:
- If a child has exited: returns its pid, fills status
- If no child has exited yet: returns 0

Loop `waitpid(WNOHANG)` in the handler until it returns 0 — this reaps all children that have exited since the last SIGCHLD delivery. (Multiple children can exit before one SIGCHLD is delivered — signals don't queue.)

---

## 9. Why `dup2` for redirection?

File descriptors are just integers. `STDIN_FILENO = 0`, `STDOUT_FILENO = 1`, `STDERR_FILENO = 2`.

`dup2(fd, 1)` makes fd 1 (stdout) point to the same open file description as `fd`. After `dup2`, writing to fd 1 writes to the file. The program being `execve`'d doesn't know or care — it just writes to stdout.

This is the mechanism behind `ls > file.txt`. The shell opens `file.txt`, gets fd=5, calls `dup2(5, 1)`, closes 5, then `execve("ls")`. `ls` writes to fd 1 (stdout), which now points to `file.txt`.

---

## 10. Why `malloc` for getcwd in `command_pwd`?

`getcwd(NULL, 0)` is a Linux extension (POSIX allows it) that dynamically allocates exactly as much memory as needed. Alternative: `char buf[PATH_MAX]` — wastes 4096 bytes on stack, and `PATH_MAX` isn't guaranteed to be large enough on all systems.

Dynamic allocation is correct, just remember to `free(cwd)` after use.

---

## 11. Error handling: `perror` vs `fprintf(stderr)`

`perror("fork")` prints `"fork: No such process"` — the string plus the human-readable errno. Use for syscall failures where errno is meaningful.

`fprintf(stderr, "my_shell: %s: command not found\n", cmd)` — use for logic errors where errno is irrelevant or misleading.

Rule: syscall fails → `perror`. Logic error → `fprintf(stderr)`. Never `printf` for errors.
