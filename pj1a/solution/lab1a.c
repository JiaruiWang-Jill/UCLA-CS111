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

/* options descriptor */
static struct option const long_opts[] = {
    {"shell", required_argument, NULL, SHELL_SHORT_OPTION},
    {"debug", no_argument, NULL, DEBUG_SHORT_OPTION},
    {0, 0, 0, 0},
};

void usage();

struct termios orig_termios_p;
void tc_setup();
void tc_reset();

void exit_success();
void exit_failure();

void sigpipe_handler(int signal);

int main(int argc, char *argv[]) {
  // create a backup of an origial termios_p
  tcgetattr(STDIN_FILENO, &orig_termios_p);

  bool shell_set = false, debug = false;
  char shell_program[256];

  tc_setup();

  /* option parsing */
  int optc;
  while ((optc = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) {
    switch (optc) {
      case SHELL_SHORT_OPTION:
        // use the given program as the shell
        shell_set = true;
        strcpy(shell_program, optarg);
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
  // --shell option is required
  if (!shell_set) usage();
  if (debug) fprintf(stderr, "shell_program: %s\r\n", shell_program);

  /* initializing pipes */
  int t2s_fd[2];  // terminal-to-shell pipe
  int *from_t_fd = &t2s_fd[0];
  int *to_s_fd = &t2s_fd[1];
  int s2t_fd[2];  // shell-to-terminal pipe
  int *from_s_fd = &s2t_fd[0];
  int *to_t_fd = &s2t_fd[1];
  if (pipe(t2s_fd) == -1 || pipe(s2t_fd) == -1) {
    fprintf(stderr, "Failed to create pipes: %s\r\n", strerror(errno));
    exit_failure();
  }

  /* forking */
  int child_pid = fork();
  if (child_pid == -1) {
    fprintf(stderr, "Failed to fork: %s\r\n", strerror(errno));
    exit_failure();
  }
  const bool is_parent = (child_pid > 0);
  const bool is_child = (child_pid == 0);

  /* parent (terminal) process */
  if (is_parent) {
    if (debug) fprintf(stderr, "[parent] parent alive\r\n");

    // close unused end of pipes
    if (close(*from_t_fd)) {
      // no need to read from itself
      fprintf(stderr, "Failed to close from_t_fd: %s\r\n", strerror(errno));
      exit_failure();
    }
    if (close(*to_t_fd)) {
      // not need to write to itself
      fprintf(stderr, "Failed to close to_t_fd: %s\r\n", strerror(errno));
      exit_failure();
    }

    char buf[BUF_SIZE];
    char c;
    int count;

    struct pollfd pollfds[2];
    pollfds[0].fd = STDIN_FILENO;
    pollfds[0].events = POLLIN + POLLHUP + POLLERR;
    pollfds[1].fd = *from_s_fd;
    pollfds[1].events = POLLIN + POLLHUP + POLLERR;

    signal(SIGPIPE, sigpipe_handler);

    while (true) {
      if (poll(pollfds, 2, -1) == -1) {
        fprintf(stderr, "Failed to poll: %s\r\n", strerror(errno));
        exit_failure();
      }
      if (pollfds[0].revents & POLLIN) {
        // handle keyboard input
        count = read(STDIN_FILENO, buf, sizeof(buf));
        for (int i = 0; i < count; i++) {
          c = buf[i];
          switch (c) {
            case EOT:
              if (debug) fprintf(stderr, "[parent] stdin: EOT\r\n");
              write(STDOUT_FILENO, EOT_REPR, sizeof(EOT_REPR));
              close(*to_s_fd);
              exit_success();
            case INTERRUPT:
              if (debug) fprintf(stderr, "[parent] stdin: INTERRUPT\r\n");
              write(STDOUT_FILENO, INTERRUPT_REPR, sizeof(INTERRUPT_REPR));
              close(*to_s_fd);
              kill(child_pid, SIGINT);
              exit_success();
            case '\r':
            case '\n':
              if (c == '\r' && debug)
                fprintf(stderr, "[parent] stdin: carriage return\r\n");
              if (c == '\n' && debug)
                fprintf(stderr, "[parent] stdin: line feed\r\n");
              write(STDOUT_FILENO, CRLF, sizeof(CRLF));
              write(*to_s_fd, &LF, sizeof(LF));
              break;
            default:
              write(STDOUT_FILENO, &c, sizeof(char));
              write(*to_s_fd, &c, sizeof(char));
          }
        }
      }
      if (pollfds[1].revents & POLLIN) {
        // handle shell output
        count = read(*from_s_fd, buf, sizeof(buf));
        if (count > 0 && debug)
          fprintf(stderr, "[parent] from_shell: incoming content\r\n");
        for (int i = 0; i < count; i++) {
          c = buf[i];
          if (c == '\r') {
            if (debug)
              fprintf(stderr, "[parent] from_shell: carriage return\r\n");
            write(STDOUT_FILENO, CRLF, sizeof(CRLF));
          } else {
            write(STDOUT_FILENO, &c, sizeof(char));
          }
        }
      }

      if (pollfds[0].revents & (POLLHUP | POLLERR)) {
        if ((pollfds[0].revents & POLLHUP) && debug)
          fprintf(stderr, "[parent] terminal POLLHUP\r\n");
        if ((pollfds[0].revents & POLLERR) && debug)
          fprintf(stderr, "[parent] terminal POLLERR\r\n");
        // TODO: handle final output
        int status = 0;
        waitpid(child_pid, &status, 0);
        printf("SHELL EXIT SIGNAL=%d STATUS=%d \r\n", status & 0x7f,
               status & 0xff00);
        exit_failure();
      }

      if (pollfds[1].revents & (POLLHUP | POLLERR)) {
        if ((pollfds[1].revents & POLLHUP) && debug)
          fprintf(stderr, "[parent] shell POLLHUP\r\n");
        if ((pollfds[1].revents & POLLERR) && debug)
          fprintf(stderr, "[parent] shell POLLERR\r\n");
        // TODO: handle final output
        int status = 0;
        waitpid(child_pid, &status, 0);
        printf("SHELL EXIT SIGNAL=%d STATUS=%d \r\n", status & 0x7f,
               status & 0xff00);
        exit_failure();
      }
    }
  }

  /* child (shell) process */
  if (is_child) {
    if (debug) fprintf(stderr, "[child] child created\r\n");

    // close unused end of pipes
    close(*from_s_fd);  // no need to read from itself
    close(*to_s_fd);    // no need to write to itself

    // redirect the output of terminal-to-shell pipe to stdin
    if (close(STDIN_FILENO)) {
      fprintf(stderr, "Failed to close stdin: %s\r\n", strerror(errno));
      exit_failure();
    }
    if (dup2(*from_t_fd, STDIN_FILENO) == -1) {
      fprintf(stderr, "Failed to dup from_t_fd: %s\r\n", strerror(errno));
      exit_failure();
    }
    // close the original terminal-to-shell read end
    if (close(*from_t_fd)) {
      fprintf(stderr, "Failed to close from_t_fd: %s\r\n", strerror(errno));
      exit_failure();
    }
    if (debug) fprintf(stderr, "[child] in pipe redirected to stdin\r\n");

    // redirect stdout to the input of shell-to-terminal pipe
    if (close(STDOUT_FILENO)) {
      fprintf(stderr, "Failed to close stdin: %s\r\n", strerror(errno));
      exit_failure();
    }
    if (dup2(*to_t_fd, STDOUT_FILENO) == -1) {
      fprintf(stderr, "Failed to dup to_t_fd: %s\r\n", strerror(errno));
      exit_failure();
    }
    if (debug) fprintf(stderr, "[child] out pipe redirected to stdout\r\n");

    // redirect stderr to the input of shell-to-terminal pipe
    if (close(STDERR_FILENO)) {
      fprintf(stdout, "Failed to close stderr: %s\r\n", strerror(errno));
      exit_failure();
    }
    if (dup2(*to_t_fd, STDERR_FILENO) == -1) {
      fprintf(stdout, "Failed to dup to_t_fd: %s\r\n", strerror(errno));
      // here the error message can only be written to stdout,
      // since stderr has been closed
      exit_failure();
    }
    if (debug) fprintf(stdout, "[child] out pipe redirected to stderr\r\n");
    // close the original shell-to-terminal write end
    if (close(*to_t_fd)) {
      fprintf(stderr, "Failed to close to_t_fd: %s\r\n", strerror(errno));
      exit_failure();
    }

    if (execlp(shell_program, shell_program, NULL) == -1) {
      fprintf(stderr, "Failed to execute %s: %s\r\n", shell_program,
              strerror(errno));
      exit_failure();
    }
  }

  return 0;
}

void tc_setup() {
  struct termios termios_p;
  tcgetattr(STDIN_FILENO, &termios_p);
  termios_p.c_iflag = ISTRIP;  // only lower 7 bits
  termios_p.c_oflag = 0;       // no processing
  termios_p.c_lflag = 0;       // no processing
  tcsetattr(STDIN_FILENO, TCSANOW, &termios_p);
}
void tc_reset() { tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios_p); }

void exit_success() {
  tc_reset();
  exit(EXIT_SUCCESS);
}

void exit_failure() {
  tc_reset();
  exit(EXIT_FAILURE);
}

void usage() {
  fprintf(stderr,
          "\n\
Usage: %s --shell=PROGRAM\n\
\n\
",
          program_name);
  fputs(
      "\
--shell=PROGRAM       use the specified program as a shell\n\
\n\
",
      stderr);
  exit_failure();
}

void sigpipe_handler(int signal) {
  fprintf(stderr, "SIGPIPE received. signal: %d\r\n", signal);
  exit_success();
}

