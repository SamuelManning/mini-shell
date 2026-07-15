// mini-shell: a small Unix shell, built to learn C++ + process/OS fundamentals.
//
// What already works in this starter:
//   - prints a prompt, reads a line of input
//   - splits it into words (tokens)
//   - runs built-ins: cd, exit
//   - for anything else: forks a child process and execvp()'s the command
//   - parent waits for the child to finish
//
// What YOU implement next (see README.md "Next Steps" for the plan):
//   - pipes:        ls | grep foo
//   - redirection:  ls > out.txt        cat < in.txt
//   - background jobs (stretch goal): sleep 5 &
//
// Don't skip straight to an AI-generated answer for those — wire up execvp()
// and fork() for one built-in above first so you understand what's happening,
// then extend this same loop.

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

// Splits a raw input line into a vector of tokens (very simple whitespace split
// for now -- doesn't yet handle quoted strings, | or > as separate tokens).
std::vector<std::string> tokenize(const std::string &line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) {
        tokens.push_back(tok);
    }
    return tokens;
}

// Converts our vector<string> into the char* argv[] execvp() expects.
// The returned pointers are only valid as long as `tokens` is alive.
std::vector<char *> toArgv(std::vector<std::string> &tokens) {
    std::vector<char *> argv;
    for (auto &t : tokens) {
        argv.push_back(&t[0]);
    }
    argv.push_back(nullptr); // execvp requires a null-terminated array
    return argv;
}

int main() {
    std::string line;

    while (true) {
        std::cout << "mini-shell> " << std::flush;

        if (!std::getline(std::cin, line)) {
            std::cout << std::endl; // handle Ctrl-D (EOF) cleanly
            break;
        }

        if (line.empty()) {
            continue;
        }

        std::vector<std::string> tokens = tokenize(line);
        if (tokens.empty()) {
            continue;
        }

        const std::string &cmd = tokens[0];

        // --- Built-ins: these MUST run in the shell's own process, not a
        // child, because "cd" changes the shell's working directory. ---
        if (cmd == "exit") {
            break;
        }

        if (cmd == "cd") {
            if (tokens.size() < 2) {
                std::cerr << "cd: expected an argument" << std::endl;
            } else if (chdir(tokens[1].c_str()) != 0) {
                perror("cd");
            }
            continue;
        }

        // --- Everything else: fork + exec ---
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // Child process
            std::vector<char *> argv = toArgv(tokens);
            execvp(argv[0], argv.data());
            // execvp only returns if it failed
            perror("mini-shell");
            _exit(127);
        } else {
            // Parent process: wait for the child to finish before
            // showing the next prompt.
            int status;
            waitpid(pid, &status, 0);
        }
    }

    return 0;
}
