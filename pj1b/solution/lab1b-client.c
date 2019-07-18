/*
 * lab1b.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <mcrypt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define PROGRAM_NAME "lab1b-client"

#define AUTHORS proper_name("Qingwei Zeng")

#define EOT 0x04        // ctrl + D, i.e. End-of-Transmission character (EOT)
#define INTERRUPT 0x03  // ctrl + C, i.e. the interrupt character
#define BUF_SIZE 256
#define KEY_MAX_LENGTH 1024

#define LOG_SEND true
#define LOG_RECEIVE false

/* For long options that have no equivalent short option, use a
  non-character as a pseudo short option, starting with CHAR_MAX + 1.
  Here, we use CHAR_MAX = 255 */
#define HOST_SHORT_OPTION 256
#define PORT_SHORT_OPTION 257
#define LOG_SHORT_OPTION 258
#define ENCRYPT_SHORT_OPTION 259
#define DEBUG_SHORT_OPTION 260

char const program_name[] = PROGRAM_NAME;

/* syscall check function
 * checks the return value of a syscall and prints error message to stderr */
void _c(int ret, char *errmsg);

char const EOT_REPR[] = "^D";
char const INTERRUPT_REPR[] = "^C";
char const CRLF[] = "\r\n";
char const LF[] = "\n";

bool host_set = false, port_set = false, log_set = false, encrypt_set = false,
     debug = false;

int logfd;
char log_prefix_buf[256];
char const LOG_SENT_PREFIX[] = "SENT %ld bytes: ";
char const LOG_RECEIVED_PREFIX[] = "RECEIVED %ld bytes: ";

void debug_write(char const *buf, int size) {
  if (debug) _c(write(STDERR_FILENO, buf, size), "Failed to write debug log");
};

void debug_printf(char const *string) {
  if (debug) fprintf(stderr, "%s", string);
}

void write_log(char const *buf, size_t size, bool flag) {
  // flag: false for receive log, true for send log
  sprintf(log_prefix_buf, flag ? LOG_SENT_PREFIX : LOG_RECEIVED_PREFIX, size);
  _c(write(logfd, log_prefix_buf, strlen(log_prefix_buf)),
     "Failed to write log prefix");
  _c(write(logfd, buf, size), "Failed to write encrypted cotent to log");
  _c(write(logfd, LF, 1), "Failed to write new line to log");
}

void sigpipe_handler(int sig) {
  fprintf(stderr, "SIGPIPE received. signal: %d\r\n", sig);
  exit(EXIT_SUCCESS);
}

MCRYPT session;
MCRYPT init_mcrypt_session(char *key_pathname);
void close_mcrypt_session(MCRYPT session);

int read_socket(int sockfd, void *buf, size_t size);
void write_socket(int sockfd, void const *buf, size_t size);

void usage();

void exit_cleanup();

struct termios orig_termios_attr;
void tc_reset();
void tc_setup();

int client_connect(char *host_name, unsigned int portno);

int main(int argc, char *argv[]) {
  // create a backup of an origial termios_attr
  tcgetattr(STDIN_FILENO, &orig_termios_attr);

  /* options descriptor */
  static struct option const long_opts[] = {
      {"host", required_argument, NULL, HOST_SHORT_OPTION},
      {"port", required_argument, NULL, PORT_SHORT_OPTION},
      {"log", required_argument, NULL, LOG_SHORT_OPTION},
      {"encrypt", required_argument, NULL, ENCRYPT_SHORT_OPTION},
      {"debug", no_argument, NULL, DEBUG_SHORT_OPTION},
      {0, 0, 0, 0},
  };

  unsigned int portno;
  char host_name[256] = "localhost";  // default to localhost
  char log_pathname[256];
  char key_pathname[256];

  /* option parsing */
  int optc;
  while ((optc = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) {
    switch (optc) {
      case HOST_SHORT_OPTION:
        // use the given program as the socket
        host_set = true;
        strcpy(host_name, optarg);
        break;
      case PORT_SHORT_OPTION:
        // use the given program as the socket
        port_set = true;
        portno = atoi(optarg);
        break;
      case LOG_SHORT_OPTION:
        log_set = true;
        strcpy(log_pathname, optarg);
        break;
      case ENCRYPT_SHORT_OPTION:
        encrypt_set = true;
        strcpy(key_pathname, optarg);
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

  if (!port_set) {
    fputs("--port option is mandatory.\r\n", stderr);
    usage();
  }

  if (log_set)
    _c((logfd = open(log_pathname, O_WRONLY | O_CREAT,
                     S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)),
       "Failed to open log file");

  if (encrypt_set) init_mcrypt_session(key_pathname);

  tc_setup();

  atexit(exit_cleanup);

  char buf[512];
  ssize_t count;

  int sockfd = client_connect(host_name, portno);
  struct pollfd pollfds[2];
  pollfds[0].fd = STDIN_FILENO;
  pollfds[0].events = POLLIN + POLLHUP + POLLERR;
  pollfds[1].fd = sockfd;
  pollfds[1].events = POLLIN + POLLHUP + POLLERR;

  signal(SIGPIPE, sigpipe_handler);

  while (true) {
    _c(poll(pollfds, 2, -1), "Failed to poll stdin and from_server");

    /* process keyboard inputs and forward them to socket */
    if (pollfds[0].revents & POLLIN) {
      _c(count = read(STDIN_FILENO, buf, sizeof(buf)),
         "Failed to read from stdin");
      for (ssize_t i = 0; i < count; i++) switch (buf[i]) {
          case '\r':
          case '\n':
            _c(write(STDOUT_FILENO, CRLF, 2), "Failed to write to stdout");
            write_socket(sockfd, LF, sizeof(LF));
            break;
          default:
            _c(write(STDOUT_FILENO, &buf[i], 1), "Failed to write to stdout");
            write_socket(sockfd, &buf[i], 1);
        }
    }

    /* process socket outputs and forward them to stdout */
    if (pollfds[1].revents & POLLIN) {
      count = read_socket(sockfd, buf, sizeof(buf));
      if (count == 0) exit(EXIT_SUCCESS);
      for (ssize_t i = 0; i < count; i++) switch (buf[i]) {
          case EOT:
            _c(write(STDOUT_FILENO, &buf[i], 1), "Failed to write to stdout");
            exit(EXIT_SUCCESS);
            break;
          default:
            _c(write(STDOUT_FILENO, &buf[i], 1), "Failed to write to stdout");
        }
    }

    if (pollfds[0].revents & (POLLHUP | POLLERR)) {
      if (pollfds[0].revents & POLLERR)
        debug_printf("stdin has poll error.\r\n");
      if (pollfds[0].revents & POLLHUP)
        debug_printf("stdin socket has hung up.\r\n");
      count = read_socket(sockfd, buf, sizeof(buf));
      if (count == 0) exit(EXIT_SUCCESS);
      for (ssize_t i = 0; i < count; i++) switch (buf[i]) {
          case '\n':
            _c(write(STDOUT_FILENO, CRLF, 2), "Failed to write to stdout");
            break;
          default:
            _c(write(STDOUT_FILENO, &buf[i], 1), "Failed to write to stdout");
        }
      exit(EXIT_SUCCESS);
    }

    if (pollfds[1].revents & (POLLHUP | POLLERR)) {
      if (pollfds[1].revents & POLLERR)
        debug_printf("socket fd has poll err.\r\n");
      if (pollfds[1].revents & POLLHUP)
        debug_printf("socket fd has hung up.\r\n");
      exit(EXIT_SUCCESS);
    }
  }
  exit(EXIT_SUCCESS);
}

int client_connect(char *host_name, unsigned int portno) {
  int sockfd;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  _c((sockfd = socket(AF_INET, SOCK_STREAM, 0)), "Failed to open socket");
  if ((server = gethostbyname(host_name)) == NULL) {
    fprintf(stderr, "Failed to find server with name %s", host_name);
    exit(EXIT_FAILURE);
  }
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;  // IPv4 address family
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(portno);
  _c(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)),
     "Failed to connect to server");  // initiate the connection to server
  return sockfd;
}

void exit_cleanup() {
  tc_reset();
  if (log_set) _c(close(logfd), "Failed to close the log file");
  if (encrypt_set) close_mcrypt_session(session);
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
}

void usage() {
  // TODO: rewrite the usage message
  exit(EXIT_FAILURE);
}

void _c(int ret, char *errmsg) {
  if (ret != -1) return;  // syscall suceeded
  fprintf(stderr, "%s: %s. errno %d\r\n", errmsg, strerror(errno), errno);
  exit(EXIT_FAILURE);
}

MCRYPT init_mcrypt_session(char *key_pathname) {
  char keystr[KEY_MAX_LENGTH];
  int keyfd, keylen;
  _c(keyfd = open(key_pathname, O_RDONLY), "Failed to open key file");
  // read key from the specified file into key_buf
  _c(keylen = read(keyfd, keystr, KEY_MAX_LENGTH),
     "Failed to read from key file");
  _c(close(keyfd), "Failed to close key file");
  session = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  char *iv = malloc(mcrypt_enc_get_iv_size(session));
  memset(iv, 0, mcrypt_enc_get_iv_size(session));
  mcrypt_generic_init(session, keystr, keylen, iv);
  return session;
}

void close_mcrypt_session(MCRYPT session) {
  mcrypt_generic_deinit(session);
  mcrypt_module_close(session);
}
int read_socket(int sockfd, void *buf, size_t size) {
  ssize_t count;
  _c(count = recv(sockfd, buf, size, 0),
     "Failed to read from server socket buffer");
  if (count == 0) return 0;
  if (log_set) write_log(buf, count, LOG_RECEIVE);
  if (encrypt_set) mdecrypt_generic(session, buf, count);
  return count;
}

void write_socket(int sockfd, void const *buf, size_t size) {
  if (encrypt_set) {
    char buf_encrypted[256];
    memcpy(buf_encrypted, buf, size);
    mcrypt_generic(session, buf_encrypted, size);
    if (log_set) write_log(buf_encrypted, size, LOG_SEND);
    _c(send(sockfd, buf_encrypted, size, 0), "Failed to send to socket buffer");
  } else {
    _c(send(sockfd, buf, size, 0), "Failed to send to socket buffer");
    if (log_set) write_log(buf, size, LOG_SEND);
  }
}
