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
#include <fcntl.h>

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

// Looks for a ">" token. If found, removes it and the filename that follows
// it from `tokens`, and returns the filename separately. Returns "" if no
// redirection was requested.
std::string extractOutputRedirect(std::vector<std::string> &tokens) {
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == ">") {
            if (i + 1 >= tokens.size()) {
                std::cerr << "mini-shell: expected filename after '>'" << std::endl;
                return "";
            }
            std::string filename = tokens[i + 1];
            tokens.erase(tokens.begin() + i, tokens.begin() + i + 2); // remove ">" and filename
            return filename;
        }
    }
    return "";
}

std::string extractInputRedirect(std::vector<std::string> &tokens) {
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "<") {
            if (i + 1 >= tokens.size()) {
                std::cerr << "mini-shell: expected filename after '<'" << std::endl;
                return "";
            }
            std::string filename = tokens[i + 1];
            tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
            return filename;
        }
    }
    return "";
}

// Returns true if a "|" was found in tokens, and splits everything into
// two separate command vectors — one for each side of the pipe.
bool splitOnPipe(const std::vector<std::string> &tokens,
                  std::vector<std::string> &left,
                  std::vector<std::string> &right) {
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "|") {
            left.assign(tokens.begin(), tokens.begin() + i);
            right.assign(tokens.begin() + i + 1, tokens.end());
            return true;
        }
    }
    return false;
}

// Runs two commands connected by a pipe: left's output feeds right's input.
void runPipeline(std::vector<std::string> &leftTokens, std::vector<std::string> &rightTokens) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        // First child: its OUTPUT goes into the pipe instead of the screen
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        std::vector<char *> argv = toArgv(leftTokens);
        execvp(argv[0], argv.data());
        perror("mini-shell");
        _exit(127);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // Second child: its INPUT comes from the pipe instead of the keyboard
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        std::vector<char *> argv = toArgv(rightTokens);
        execvp(argv[0], argv.data());
        perror("mini-shell");
        _exit(127);
    }

    // Parent doesn't use the pipe itself — must close both ends, or the
    // second command will hang forever waiting for input that never
    // technically "ends".
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
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
        std::vector<std::string> leftTokens, rightTokens;
        if (splitOnPipe(tokens, leftTokens, rightTokens)) {
            runPipeline(leftTokens, rightTokens);
            continue;
        }
        std::string outputFile = extractOutputRedirect(tokens);
        std::string inputFile = extractInputRedirect(tokens);
        if (tokens.empty()) {
            continue; // handled a line that was just "> file" with nothing else
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
            if (!outputFile.empty()) {
                int fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("open");
                    _exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            if (!inputFile.empty()) {
                int fd = open(inputFile.c_str(), O_RDONLY);
                if (fd < 0) {
                    perror("open");
                    _exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
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
