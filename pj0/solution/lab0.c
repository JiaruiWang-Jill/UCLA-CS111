/*
 * lab0.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PROGRAM_NAME "lab0"

#define AUTHORS proper_name ("Qingwei Zeng")

int  use_file_input   = 0;
int  use_file_output  = 0;
int  force_segfault   = 0;
int  catch            = 0;

char ch;

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try `%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION]... [FILE]\n\
  or:  %s -e [OPTION]... [ARG]...\n\
  or:  %s -i LO-HI [OPTION]...\n\
"),
              program_name, program_name, program_name);
      fputs (_("\
Write a random permutation of the input lines to standard output.\n\
\n\
"), stdout);
      fputs (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
"), stdout);
      fputs (_("\
  -e, --echo                treat each ARG as an input line\n\
  -i, --input-range=LO-HI   treat each number LO through HI as an input line\n\
  -n, --head-count=COUNT    output at most COUNT lines\n\
  -o, --output=FILE         write result to FILE instead of standard output\n\
      --random-source=FILE  get random bytes from FILE\n\
  -z, --zero-terminated     end lines with 0 byte, not newline\n\
"), stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      fputs (_("\
\n\
With no FILE, or when FILE is -, read standard input.\n\
"), stdout);
      emit_ancillary_info ();
    }

  exit (status);
}


int
main (int argc, char *argv[])
{

  int input_fd  = STDIN_FILENO;    // default to STDIN
  int output_fd = STDOUT_FILENO;   // default to STDOUT

  /* options descriptor */
  static struct option longopts[] = {
    { "input",       optional_argument,    NULL,               'i' },
    { "output",      optional_argument,    NULL,               'o' },
    { "segfault",    optional_argument,    &force_segfault,    1 },
    { "catch",       optional_argument,    &catch,             1 },
  };

  while ((ch = getopt_long(argc, argv, "io:", longopts, NULL)) != -1) {
    switch (ch) {
      case 'i':
        printf("input:"
            " %s\n", optarg);
        if ((input_fd = open(optarg, O_RDONLY, 0)) == -1) {
          fprintf(stderr, "Unable to open the given input file: %s. Error: ",
              optarg);
          /*
          switch (errno) {
            case EACCES:
              fprintf(stderr, "permission denied.");
              break;
            case EAGAIN:
              fprintf(stderr, "specified path is the slave side of a locked "
                  "pseudo-terminal device.");
              break;

          }
          */
          exit(2);
        }
        break;
      case 'o':
        printf("output: %s\n", optarg);
        if ((output_fd = open(optarg, O_WRONLY, 0)) == -1) {
          fprintf(stderr, "Unable to open the given output file: %s\n", optarg);
          exit(3);
        }
        break;
      case 0:
        if (force_segfault) {
          fprintf(stderr,"Buffy will use her dagger to "
              "apply fluoride to dracula's teeth\n");
          break;
        }
        if (catch) {
          fprintf(stderr,"fuck now\n");
          break;
        }
    }
  }

  exit(EXIT_SUCCESS);
}

