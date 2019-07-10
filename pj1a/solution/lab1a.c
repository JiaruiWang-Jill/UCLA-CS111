/*
 * lab1a.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <getopt.h>
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

#define EOT 0x04  // ctrl + D, i.e. End-of-Transmission character (EOT)
#define BUF_SIZE 256

/* For long options that have no equivalent short option, use a
  non-character as a pseudo short option, starting with CHAR_MAX + 1.
  Here, we use CHAR_MAX = 255 */
#define SHELL_SHORT_OPTION 256

char const program_name[] = PROGRAM_NAME;

char const EOT_REPR[] = "^D";
char const CRLF[] = "\r\n";

void usage(int status) {
  fprintf(stderr,
          "\n\
Usage: %s\n\
       %s --shell=PROGRAM\n\
\n\
",
          program_name, program_name);
  fputs(
      "\
Pipe a designated input into a designated output.\n\
\n\
",
      stderr);
  fputs(
      "\
--shell=PROGRAM       use the specified program as a shell\n\
",
      stderr);
  fputs(
      "\
\n\
With no --shell option, it reads (writes) standard input (output).\n\
",
      stderr);
  exit(status);
}

int main(int argc, char *argv[]) {
  struct termios orig_termios_p, new_termios_p;
  bool shell_set = false;
  char shell_program[64];

  /* options descriptor */
  static struct option const long_opts[] = {
      {"shell", required_argument, NULL, SHELL_SHORT_OPTION},
      {0, 0, 0, 0},
  };

  tcgetattr(STDIN_FILENO, &orig_termios_p);  // keey a backup

  tcgetattr(STDIN_FILENO, &new_termios_p);
  new_termios_p.c_iflag = ISTRIP;  // only lower 7 bits
  new_termios_p.c_oflag = 0;       // no processing
  new_termios_p.c_lflag = 0;       // no processing
  tcsetattr(STDIN_FILENO, TCSANOW, &new_termios_p);

  int optc;
  /* option parsing */
  while ((optc = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) {
    switch (optc) {
      case SHELL_SHORT_OPTION:
        /* use the given program as the shell */
        shell_set = true;
        strcpy(optarg, shell_program);
        break;
      default:
        usage(EXIT_FAILURE);
    }
  }

  int proc_id;

  if (shell_set) proc_id = fork();
  if (proc_id == -1) {
    // TODO: check errno
    // TODO: exit
  }

  // fork was successful, create pipes

  int ttos_fd[2];  // terminal-to-shell pipe
  int *to_s_fd = &ttos_fd[0];
  int *from_t_fd = &ttos_fd[1];
  if (pipe(ttos_fd)) {
    // TODO: error happened! check errno
    // TODO: exit
  }

  int stot_fd[2];  // shell-to-terminal pipe
  int *to_t_fd = &stot_fd[0];
  int *from_s_fd = &stot_fd[1];
  if (pipe(stot_fd)) {
    // TODO: error happened! check errno
    // TODO: exit
  }

  const bool is_parent = (proc_id > 0);
  const bool is_child = (proc_id == 0);

  /* parent (terminal) process */
  if (is_parent) {
    // close unused end of pipes
    close(*from_t_fd);  // no need to read from itself
    close(*to_t_fd);    // not need to write to itself

    char buf[BUF_SIZE];
    char c;
    while (true) {
      // handle keyboard input
      read(STDIN_FILENO, buf, sizeof(buf));

      // TODO: check errno
      for (int i = 0; i < BUF_SIZE; i++) {
        c = buf[i];
        if (c == EOT) {
          write(*to_s_fd, EOT_REPR, sizeof(EOT_REPR));
          // TODO: check errno
          tcsetattr(*to_s_fd, TCSANOW, &orig_termios_p);
          exit(EXIT_SUCCESS);
        }
        // restore terminal, exit.
        else if (c == '\r' || c == '\n')
          write(*to_s_fd, CRLF, sizeof(CRLF));
        else
          // write to shell, if a shell is specified; otherwise, write to stdout
          write(*to_s_fd, &c, sizeof(char));
      }

      // handle shell-to-terminal pipe
      read(*from_s_fd, buf, sizeof(buf));
      /* handle “\n”, forward to stdout */
    }
    /* int status = 0; */
    /* waitpid(proc_id, &status, 0); */
    /* printf("Child process exits with code : % d\n", (status & 0xff)); */
  }

  /* child (shell) process */
  if (is_child) {
    // close unused end of pipes
    close(*from_s_fd);  // no need to read from itself
    close(*to_s_fd);    // no need to write to itself

    // redirect the output of terminal-to-shell pipe to stdin
    close(STDIN_FILENO);
    dup(*from_t_fd);
    // redirect stdout and stderr to the input of shell-to-terminal pipe
    close(STDERR_FILENO);
    dup(*to_t_fd);
    close(STDOUT_FILENO);
    dup(*to_t_fd);

    execlp(shell_program, shell_program, NULL);
  }

  return 0;
}
