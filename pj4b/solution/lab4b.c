/*
 * lab4b.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <mraa.h>
#include <mraa/aio.h>

#include <ctype.h>
#include <getopt.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PROGRAM_NAME "lab4b"
#define AUTHORS proper_name("Qingwei Zeng")

char const program_name[] = PROGRAM_NAME;

#define A0 1
#define GPIO_50 60

#define PERIOD_SHORT_OPTION 256
#define SCALE_SHORT_OPTION 257
#define LOG_SHORT_OPTION 258

struct timeval m_clock;

char scale = 'F';
int period = 1;
FILE *log_file = NULL;
int report = true;
time_t next_report_time = 0;

void usage();

void log_to_file(char *str);

float get_temperature();

bool starts_with(const char *a, const char *b) {
  if (strncmp(a, b, strlen(b)) == 0) return true;
  return false;
}

mraa_aio_context sensor;
mraa_gpio_context button;

void shutdown() {
  struct tm *now;
  now = localtime(&m_clock.tv_sec);
  char msg[256];
  sprintf(msg, "%02d:%02d:%02d SHUTDOWN", now->tm_hour, now->tm_min,
          now->tm_sec);
  printf("%s\n", msg);
  log_to_file(msg);
  exit(EXIT_SUCCESS);
}

void process_stdin(char *input) {
  while (*input == ' ' || *input == '\t') input++;

  if (strcmp(input, "SCALE=F") == 0) {
    log_to_file(input);
    scale = 'F';
  }
  if (strcmp(input, "SCALE=C") == 0) {
    log_to_file(input);
    scale = 'C';
  }
  if (strcmp(input, "STOP") == 0) {
    log_to_file(input);
    report = false;
  }
  if (strcmp(input, "START") == 0) {
    log_to_file(input);
    report = true;
  }
  if (strcmp(input, "OFF") == 0) {
    log_to_file(input);
    shutdown();
  }
  if (starts_with(input, "PERIOD=")) {
    char *n = &input[7];
    period = atoi(n);
    log_to_file(input);
  }
  if (starts_with(input, "LOG")) log_to_file(input);
}

int main(int argc, char *argv[]) {
  struct option long_opts[] = {
      {"period", required_argument, NULL, PERIOD_SHORT_OPTION},
      {"scale", required_argument, NULL, SCALE_SHORT_OPTION},
      {"log", required_argument, NULL, LOG_SHORT_OPTION},
      {0, 0, 0, 0}};

  int optc;
  while ((optc = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
    switch (optc) {
      case PERIOD_SHORT_OPTION:
        period = atoi(optarg);
        break;
      case LOG_SHORT_OPTION:
        if ((log_file = fopen(optarg, "w+")) == NULL) {
          fprintf(stderr, "Failed to open the given log file.\n");
          exit(EXIT_FAILURE);
        }
        break;
      case SCALE_SHORT_OPTION:
        if (*optarg == 'F' || *optarg == 'C')
          scale = *optarg;
        else {
          fprintf(stderr, "Wrong temperature unit.\n");
          usage();
        }
        break;
      default:
        fprintf(stderr, "Unknown argument.\n");
        usage();
        break;
    }
  }

  if ((sensor = mraa_aio_init(A0)) == NULL) {
    fprintf(stderr, "Failed to initialize AIO A0.\n");
    exit(EXIT_FAILURE);
  }

  if ((button = mraa_gpio_init(GPIO_50)) == NULL) {
    fprintf(stderr, "Failed to initialize GPIO 50.\n");
    exit(EXIT_FAILURE);
  }

  mraa_gpio_dir(button, MRAA_GPIO_IN);
  mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &shutdown, NULL);

  struct pollfd poll_stdin;
  poll_stdin.fd = STDIN_FILENO;
  poll_stdin.events = POLLIN;

  char input[128];
  char msg[256];
  struct tm *now;
  while (true) {
    gettimeofday(&m_clock, 0);
    if (report && m_clock.tv_sec >= next_report_time) {
      int t = get_temperature() * 10;
      now = localtime(&m_clock.tv_sec);
      sprintf(msg, "%02d:%02d:%02d %d.%1d", now->tm_hour, now->tm_min,
              now->tm_sec, t / 10, t % 10);
      printf("%s\n", msg);
      log_to_file(msg);
      next_report_time = m_clock.tv_sec + period;
    }

    if (poll(&poll_stdin, 1, 0) == -1) {
      fprintf(stderr, "Failed to poll from stdin.\n");
      exit(EXIT_FAILURE);
    };

    if (poll_stdin.revents & POLLIN) {
      fgets(input, 128, stdin);
      process_stdin(input);
    }
  }

  mraa_aio_close(sensor);
  mraa_gpio_close(button);

  exit(EXIT_SUCCESS);
}

void usage() {
  fprintf(stderr,
          "\n\
Usage: %s\n\
       %s --period=SECONDS\n\
       %s --scale=[C|F]\n\
       %s --log=FILENAME\n\
\n\
",
          program_name, program_name, program_name, program_name);
  fputs(
      "\
       --period=SECONDS  the number of seconds between reporting intervals\n\
       --scale=[C|F]     unit of the temperature\n\
       --log=FILENAME    log log_file to append reports to\n\
",
      stderr);
  exit(EXIT_FAILURE);
}

void log_to_file(char *str) {
  if (log_file != NULL) {
    fprintf(log_file, "%s\n", str);
    fflush(log_file);
  }
}

/* reference
 * http://wiki.seeedstudio.com/Grove-Temperature_Sensor_V1.2/
 */
float get_temperature() {
  const int B = 4275;
  const int R0 = 100000;

  int a = mraa_aio_read(sensor);
  float R = 1023.0 / a - 1.0;
  R = R0 * R;
  float temperature = 1.0 / (log(R / R0) / B + 1 / 298.15) - 273.15;
  switch (scale) {
    case 'F':
      return (temperature * 9) / 5 + 32;
    case 'C':
      return temperature;
    default:
      fprintf(stderr, "Invalid temperature scale %c", scale);
      exit(EXIT_FAILURE);
  }
}

