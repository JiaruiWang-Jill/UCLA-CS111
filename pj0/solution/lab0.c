/*
 * lab0.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 *
 * Reference: GNU coreutils shuf by Paul Eggert
 * https://github.com/wertarbyte/coreutils/blob/master/src/shuf.c
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROGRAM_NAME "lab0"

#define AUTHORS proper_name("Qingwei Zeng")

/* For long options that have no equivalent short option, use a
  non-character as a pseudo short option, starting with CHAR_MAX + 1.
  Here, we use CHAR_MAX = 255 */
#define INPUT_SHORT_OPTION 256
#define OUTPUT_SHORT_OPTION 257
#define SEGFAULT_SHORT_OPTION 258
#define CATCH_SHORT_OPTION 259

char *const program_name = PROGRAM_NAME;

void usage(int status) {
  fprintf(stderr, "\n\
Usage: %s --input=FILE\n\
       %s --output=FILE\n\
       %s --segfault\n\
       %s --segfault --catch\n\
\n\
",
         program_name, program_name, program_name, program_name);
  fputs("\
Pipe a designated input into a designated output.\n\
\n\
",
        stderr);
  fputs("\
--input=FILE          use the specified file as input\n\
--output=FILE         create the specified file and use it as output\n\
--segfault            force a segmentation fault\n\
--catch               catch segmentation faults\n\
",
        stderr);
  fputs("\
\n\
With no --input (--output) option, it reads (writes) standard input (output).\n\
",
        stderr);
  exit(status);
}

/* Forces a segmentation fault by memcpying to NULL destination */
void segfault() {
  char * null_dest = NULL;
  memcpy(program_name, null_dest , 10);
}

void segfault_handler() {
  fprintf(stderr, "Segmentation fault was generated and caught.\n");
  exit(4);
}

int main(int argc, char *argv[]) {
  int optc;
  int in_fd = STDIN_FILENO;   // default to STDIN
  int out_fd = STDOUT_FILENO; // default to STDOUT
  char buf[1024];

  mode_t creat_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  /* options flag */
  bool segfault_set = false;

  /* options descriptor */
  static struct option const long_opts[] = {
      {"input", required_argument, NULL, INPUT_SHORT_OPTION},
      {"output", required_argument, NULL, OUTPUT_SHORT_OPTION},
      {"segfault", no_argument, NULL, SEGFAULT_SHORT_OPTION},
      {"catch", no_argument, NULL, CATCH_SHORT_OPTION},
      {0, 0, 0, 0},
  };

  /* option parsing */
  while ((optc = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) {
    switch (optc) {
    case INPUT_SHORT_OPTION:
      /* ignore potential open error here on purpose,
        since --segfault has a higher priority */
      in_fd = open(optarg, O_RDONLY);
      break;
    case OUTPUT_SHORT_OPTION:
      /* ignore potential creat error here on purpose,
        since --segfault has a higher priority */
      out_fd = open(optarg, O_CREAT | O_TRUNC | O_WRONLY, creat_mode);
      break;
    case SEGFAULT_SHORT_OPTION:
      segfault_set = true;
      break;
    case CATCH_SHORT_OPTION:
      /* register a segfault handler */
      signal(SIGSEGV, segfault_handler);
      break;
    default:
      usage(EXIT_FAILURE);
    }
  }

  /* force a segmentation fault, if --segfault is set */
  if (segfault_set)
    /* force a segmentation fault */
    segfault();

  /* handle open error caused by --input option, if any */
  if (in_fd == -1) {
    fprintf(stderr, "Failed to open the input file.\n");
    switch (errno) {
    case EEXIST:
      fprintf(stderr, "The specified file does not exist.\n");
      break;
    case EACCES:
      fprintf(stderr, "Do not have permission to read the file.\n");
      break;
    default:
      fprintf(stderr, "An error occurred. %s\n", strerror(errno));
    }
    exit(2);
  }

  /* handle creat error caused by --output option, if any */
  if (out_fd == -1) {
    fprintf(stderr, "Failed to create and open the output file.\n");
    switch (errno) {
    case EACCES:
      fprintf(stderr, "Do not have permission to create or write the file.\n");
      break;
    case EDQUOT:
      fprintf(stderr, "Insufficient disk quota.\n");
      break;
    default:
      fprintf(stderr, "An error occurred. %s\n", strerror(errno));
    }
    exit(3);
  }

  /* Piping */
  int bytes_read;
  while ((bytes_read = read(in_fd, buf, 1024)) != 0)
    write(out_fd, buf, bytes_read);

  /* clean up */
  if (close(in_fd) == -1) {
    switch (errno) {
    case EBADF:
      /* in_fd is not a valid, active file descriptor. */
      break;
    default:
      fprintf(stderr, "Failed to close the input file.\n");
    }
  }

  if (close(out_fd) == -1) {
    switch (errno) {
    case EBADF:
      /* in_fd is not a valid, active file descriptor. */
      break;
    default:
      fprintf(stderr, "Failed to close the input file.\n");
    }
  }

  exit(EXIT_SUCCESS);
}
