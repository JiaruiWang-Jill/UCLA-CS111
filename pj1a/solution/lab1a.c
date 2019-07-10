/*
 * lab1a.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define PROGRAM_NAME "lab1a"

#define AUTHORS proper_name("Qingwei Zeng")

#define EOT 0x04  // ctrl + D, i.e. End-of-Transmission character (EOT)
#define BUF_SIZE 256

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
  char c;
  char buf[BUF_SIZE];

  tcgetattr(STDIN_FILENO, &orig_termios_p);  // keey a backup

  tcgetattr(STDIN_FILENO, &new_termios_p);
  new_termios_p.c_iflag = ISTRIP;  // only lower 7 bits
  new_termios_p.c_oflag = 0;       // no processing
  new_termios_p.c_lflag = 0;       // no processing
  tcsetattr(STDIN_FILENO, TCSANOW, &new_termios_p);

  while (1) {
    read(STDIN_FILENO, buf, sizeof(buf));
    for (int i = 0; i < BUF_SIZE; i++) {
      c = buf[i];
      if (c == EOT) {
        write(STDOUT_FILENO, EOT_REPR, sizeof(EOT_REPR));
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios_p);
        exit(EXIT_SUCCESS);
      }
      // restore terminal, exit.
      else if (c == '\r' || c == '\n')
        write(STDOUT_FILENO, CRLF, sizeof(CRLF));
      else
        write(1, &c, sizeof(char));
    }
  }

  return 0;
}
