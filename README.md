# mini-shell

A small Unix shell written in C++, built to learn how a shell actually works
under the hood: process creation (`fork`), running programs (`execvp`), and
waiting for child processes (`waitpid`).

This is a learning project — the point isn't to reinvent bash, it's to
understand what bash is doing.

## What it does right now

- Prints a prompt and reads a line of input in a loop
- Splits the line into a command and its arguments
- Supports two built-ins that must run in the shell's own process:
  - `cd <dir>`
  - `exit`
- For any other command, forks a child process and runs it with `execvp`,
  then waits for it to finish before showing the next prompt

## Build & run

```bash
make
./mini-shell
```

Try it:

```
mini-shell> echo hello
hello
mini-shell> pwd
/home/you/mini-shell
mini-shell> ls -la
mini-shell> cd ..
mini-shell> exit
```

## Next steps (build these yourself, in this order)

Do these one at a time. Get each one working and committed before starting
the next — that gives you a clean commit history that actually shows how
the project developed, which is exactly what an interviewer looks at.

### 1. Output redirection: `command > file`
When a token `>` appears, everything after it is a filename. Before calling
`execvp` in the child process, open that file and use `dup2()` to redirect
`STDOUT_FILENO` to it. Look up: `open()`, `dup2()`, `O_WRONLY | O_CREAT | O_TRUNC`.

### 2. Input redirection: `command < file`
Same idea as above, but redirect `STDIN_FILENO` from a file opened with `O_RDONLY`.

### 3. Pipes: `command1 | command2`
This is the big one. You'll need to:
- split the line on `|` into separate commands first
- create a pipe with `pipe()` before forking
- fork twice (one child per command)
- wire the write end of the pipe to the first child's stdout, and the read
  end to the second child's stdin, again via `dup2()`
- make sure both children close the ends of the pipe they don't use, or the
  program will hang waiting for input that never gets closed

### 4. (Stretch) Background jobs: `command &`
Don't `waitpid()` on it immediately — track it and reap it later (or use
`waitpid` with `WNOHANG`).

## Why this project (for Qualcomm/systems-software interviews)

This one project touches: process management, the fork/exec model, file
descriptors, and the producer/consumer relationship a pipe creates between
two processes — all fair game in a systems or OS-adjacent interview, and all
things you can now speak to from something you actually built rather than
just read about.

## Known limitations (fine to leave, but be ready to mention them if asked)

- No support for quoted arguments (e.g. `echo "hello world"` is currently
  split into two tokens)
- No environment variable expansion (`$HOME`)
- No command history / arrow-key recall
