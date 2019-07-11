/*
 * lab1a.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define PROGRAM_NAME "lab1a"

#define AUTHORS proper_name("Qingwei Zeng")

#define EOT 0x04        // ctrl + D, i.e. End-of-Transmission character (EOT)
#define INTERRUPT 0x03  // ctrl + C, i.e. the interrupt character
#define BUF_SIZE 256

/* For long options that have no equivalent short option, use a
  non-character as a pseudo short option, starting with CHAR_MAX + 1.
  Here, we use CHAR_MAX = 255 */
#define SHELL_SHORT_OPTION 256
#define DEBUG_SHORT_OPTION 257

char const program_name[] = PROGRAM_NAME;

char const EOT_REPR[] = "^D";
char const INTERRUPT_REPR[] = "^C";
char const CRLF[] = "\r\n";
char const LF = '\n';

void usage();

struct termios orig_termios_attr;
void tc_reset();
void tc_setup();

void sigpipe_handler(int signal);

void wait_child(int child_pid);

/* syscall check function
 * checks the return value of a syscall and prints error message to stderr */
void _c(int ret, char *errmsg);

int main(int argc, char *argv[]) {
  // create a backup of an origial termios_attr
  tcgetattr(STDIN_FILENO, &orig_termios_attr);
  tc_setup();

  /* options descriptor */
  static struct option const long_opts[] = {
      {"shell", required_argument, NULL, SHELL_SHORT_OPTION},
      {"debug", no_argument, NULL, DEBUG_SHORT_OPTION},
      {0, 0, 0, 0},
  };

  signal(SIGPIPE, sigpipe_handler);

  bool shell_set = false, debug = false;
  char prog_name[256];

  /* option parsing */
  int optc;
  while ((optc = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) {
    switch (optc) {
      case SHELL_SHORT_OPTION:
        // use the given program as the shell
        shell_set = true;
        strcpy(prog_name, optarg);
        break;
      case DEBUG_SHORT_OPTION:
        // print debug info
        debug = true;
        break;
      default:
        fputs("Invalid arguments.\r\n", stderr);
        usage();
    }
  }

  char buf[BUF_SIZE];
  char c;
  int count;

  if (shell_set) {
    /* initializing pipes */
    int t2s_fd[2], s2t_fd[2];
    _c(pipe(t2s_fd), "Failed to create terminal-to-shell pipe");
    _c(pipe(s2t_fd), "Failed to create shell-to-terminal pipe");

    int *from_t_fd = &t2s_fd[0], *to_s_fd = &t2s_fd[1];
    int *from_s_fd = &s2t_fd[0], *to_t_fd = &s2t_fd[1];

    /* forking */
    int child_pid;
    _c(child_pid = fork(), "Failed to fork");

    bool const is_parent = (child_pid > 0);
    bool const is_child = (child_pid == 0);
    /* parent (terminal) process */
    if (is_parent) {
      // no need to read from itself
      _c(close(*from_t_fd),
         "Failed to close the read end of terminal-to-shell pipe");
      // no need to write to itself
      _c(close(*to_t_fd),
         "Failed to close the write end of shell-to-terminal pipe");

      struct pollfd pollfds[2];
      pollfds[0].fd = STDIN_FILENO;
      pollfds[0].events = POLLIN + POLLHUP + POLLERR;
      pollfds[1].fd = *from_s_fd;
      pollfds[1].events = POLLIN + POLLHUP + POLLERR;

      while (true) {
        _c(poll(pollfds, 2, -1), "Failed to poll stdin and from_shell");
        if (pollfds[0].revents & POLLIN) {
          // handle keyboard input
          _c(count = read(STDIN_FILENO, buf, sizeof(buf)),
             "Failed to read from stdin");
          for (int i = 0; i < count; i++) {
            c = buf[i];
            switch (c) {
              case EOT:
                _c(write(STDOUT_FILENO, EOT_REPR, sizeof(EOT_REPR)),
                   "Failed to write to stdout");
                _c(close(*to_s_fd),
                   "Failed to close the write end of terminal-to-shell pipe");
                // Should cause (pollfds[1].revents & POLLHUP) to be true
                break;
              case INTERRUPT:
                _c(write(STDOUT_FILENO, INTERRUPT_REPR, sizeof(INTERRUPT_REPR)),
                   "Failed to write to stdout");
                _c(kill(child_pid, SIGINT),
                   "Failed to send kill signal to child process");
                break;
              case '\r':
              case '\n':
                _c(write(STDOUT_FILENO, CRLF, sizeof(CRLF)),
                   "Failed to write to stdout");
                _c(write(*to_s_fd, &LF, sizeof(LF)),
                   "Failed to write to shell");
                break;
              default:
                _c(write(STDOUT_FILENO, &c, sizeof(char)),
                   "Failed to write to stdout");
                _c(write(*to_s_fd, &c, sizeof(char)),
                   "Failed to write to shell");
            }
          }
        }

        if (pollfds[1].revents & POLLIN) {
          // handle shell output
          _c(count = read(*from_s_fd, buf, sizeof(buf)),
             "Failed to read from shell-to-terminal pipe");
          for (int i = 0; i < count; i++) {
            c = buf[i];
            switch (c) {
              case '\n':
                _c(write(STDOUT_FILENO, CRLF, sizeof(CRLF)),
                   "Failed to write to stdout");
                break;
              case EOT:
                _c(write(STDOUT_FILENO, &c, sizeof(char)),
                   "Failed to write to stdout");
                _c(close(*from_s_fd),
                   "Failed to close the read end of shell-to-terminal pipe");
                exit(EXIT_SUCCESS);
                break;
              default:
                _c(write(STDOUT_FILENO, &c, sizeof(char)),
                   "Failed to write to stdout");
            }
          }
        }

        if (pollfds[0].revents & (POLLHUP | POLLERR)) {
          if (debug && (pollfds[0].revents & POLLERR))
            fprintf(stderr, "STDIN has poll err.\r\n");
          if (debug && (pollfds[0].revents & POLLHUP))
            fprintf(stderr, "STDIN has hung up.\r\n");
          _c(count = read(*from_s_fd, buf, sizeof(buf)),
             "Failed to read from shell-to-terminal pipe");
          for (int i = 0; i < count; i++) {
            switch (c) {
              case '\n':
                _c(write(STDOUT_FILENO, CRLF, 2 * sizeof(char)),
                   "Failed to write to stdout");
                break;
              default:
                _c(write(STDOUT_FILENO, &c, sizeof(char)),
                   "Failed to write to stdout");
            }
          }
          wait_child(child_pid);
          exit(EXIT_SUCCESS);
        }

        if (pollfds[1].revents & (POLLHUP | POLLERR)) {
          if (debug && (pollfds[1].revents & POLLERR))
            fprintf(stderr, "from_s has poll err.\r\n");
          if (debug && (pollfds[1].revents & POLLHUP))
            fprintf(stderr, "from_s has hung up.\r\n");
          _c(close(*from_s_fd),
             "Failed to close the read end of shell-to-terminal pipe");
          wait_child(child_pid);
          exit(EXIT_SUCCESS);
        }
      }
    }

    /* child (shell) process */
    if (is_child) {
      // no need to read from itself
      _c(close(*from_s_fd),
         "Failed to close the read end of shell-to-terminal pipe");
      // no need to write to itself
      _c(close(*to_s_fd),
         "Failed to close the write end of terminal-to-shell pipe");

      // redirect the output of terminal-to-shell pipe to stdin
      _c(dup2(*from_t_fd, STDIN_FILENO),
         "Failed to redirect the read end of terminal-to-shell pipe to stdin");
      // close the original terminal-to-shell read end
      _c(close(*from_t_fd),
         "Failed to close the read end of terminal-to-shell pipe");

      // redirect stdout to the input of shell-to-terminal pipe
      _c(dup2(*to_t_fd, STDOUT_FILENO),
         "Failed to redirect the write end of shell-to-terminal pipe to "
         "stdout");
      // redirect stderr to the input of shell-to-terminal pipe
      _c(dup2(*to_t_fd, STDERR_FILENO),
         "Failed to redirect the write end of shell-to-terminal pipe to "
         "stderr");
      _c(close(*to_t_fd),
         "Failed to close the write end of shell-to-terminal pipe");

      _c(execlp(prog_name, prog_name, NULL),
         "Failed to execute the designated shell program");
    }
  } else {
    while (true) {
      _c((count = read(STDIN_FILENO, buf, BUF_SIZE)),
         "Failed to read from stdin");
      for (int i = 0; i < count; i++) {
        c = buf[i];
        switch (c) {
          case EOT:
            exit(EXIT_SUCCESS);
            break;
          case '\r':
          case '\n':
            _c(write(STDOUT_FILENO, CRLF, sizeof(CRLF)),
               "Failed to write to stdout");
            break;
          default:
            _c(write(STDOUT_FILENO, &c, sizeof(c)),
               "Failed to write to stdout");
        }
      }
    }
  }
}

void tc_reset() {
  _c(tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios_attr),
     "Unable to set termios attributes");
}
void tc_setup() {
  struct termios termios_attr;
  if (tcgetattr(STDIN_FILENO, &termios_attr) == -1) {
    fprintf(stderr, "Unable to get termios attributes\n");
    exit(EXIT_FAILURE);
  }
  termios_attr.c_iflag = ISTRIP;  // only lower 7 bits
  termios_attr.c_oflag = 0;       // no processing
  termios_attr.c_lflag = 0;       // no processing
  _c(tcsetattr(STDIN_FILENO, TCSANOW, &termios_attr),
     "Unable to set termios attributes");
  atexit(tc_reset);
}

void usage() {
  fprintf(stderr,
          "\r\n\
Usage: %s --shell=PROGRAM\r\n\
\r\n\
",
          program_name);
  fputs(
      "\
--shell=PROGRAM       use the specified program as a shell\r\n\
\r\n\
",
      stderr);
  exit(EXIT_FAILURE);
}

void sigpipe_handler(int signal) {
  fprintf(stderr, "SIGPIPE received. signal: %d\r\n", signal);
  exit(EXIT_SUCCESS);
}

void wait_child(int child_pid) {
  int status = 0;
  _c(waitpid(child_pid, &status, 0), "Failed to wait for child process");
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", WTERMSIG(status),
          WEXITSTATUS(status));
}
void _c(int ret, char *errmsg) {
  if (ret != -1) return;  // syscall suceeded
  fprintf(stderr, "%s: %s. errno %d\r\n", errmsg, strerror(errno), errno);
  exit(EXIT_FAILURE);
}
