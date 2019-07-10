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

  int proc_id = -2;

  if (shell_set) proc_id = fork();
  // TODO: check errno

  int pipe_fd[2] = {STDIN_FILENO, STDOUT_FILENO};
  int *in_fd = &pipe_fd[0];
  int *out_fd = &pipe_fd[1];

  if (proc_id > 0) {
    // successfully created a child process, create pipe
    if (pipe(pipe_fd)) {
      // TODO: check errno
    }
  }

  if (proc_id != 0) {
    char buf[BUF_SIZE];
    char c;
    // parent process
    while (true) {
      read(STDIN_FILENO, buf, sizeof(buf));
      // TODO: check errno
      for (int i = 0; i < BUF_SIZE; i++) {
        c = buf[i];
        if (c == EOT) {
          write(*in_fd, EOT_REPR, sizeof(EOT_REPR));
          // TODO: check errno
          tcsetattr(*out_fd, TCSANOW, &orig_termios_p);
          exit(EXIT_SUCCESS);
        }
        // restore terminal, exit.
        else if (c == '\r' || c == '\n')
          write(*out_fd, CRLF, sizeof(CRLF));
        else
          write(1, &c, sizeof(char));
      }
    }
  } else {
    // child process
    close(to_shell[1]);
    close(0);
    dup(to_shell[0]);
    close(to_shell[0]);
    execlp(“lab0”, “lab0”, NULL);
  }

  return 0;
}
