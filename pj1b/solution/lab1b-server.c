/*
 * lab1b.c
 * Copyright (C) 2019 Qingwei Zeng <zenn@ucla.edu>
 *
 * Distributed under terms of the MIT license.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
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

#define PROGRAM_NAME "lab1b-server"

#define AUTHORS proper_name("Qingwei Zeng")

#define EOT 0x04        // ctrl + D, i.e. End-of-Transmission character (EOT)
#define INTERRUPT 0x03  // ctrl + C, i.e. the interrupt character
#define BUF_SIZE 256
#define KEY_MAX_LENGTH 1024

/* For long options that have no equivalent short option, use a
  non-character as a pseudo short option, starting with CHAR_MAX + 1.
  Here, we use CHAR_MAX = 255 */
#define PORT_SHORT_OPTION 256
#define DEBUG_SHORT_OPTION 257
#define ENCRYPT_SHORT_OPTION 258

char const program_name[] = PROGRAM_NAME;

char const shell_program[] = "/bin/bash";

char const EOT_REPR[] = "^D";
char const INTERRUPT_REPR[] = "^C";
char const CRLF[] = "\r\n";
char const LF = '\n';

bool port_set = false, encrypt_set = false, debug = false;

int sockfd;

/* syscall check function
 * checks the return value of a syscall and prints error message to stderr */
void _c(int ret, char *errmsg);

void debug_write(char const *buf, int size) {
  if (debug)
    _c(write(STDERR_FILENO, buf, size), "Failed to write debug log to stderr");
};

void debug_printf(char const *string) {
  if (debug) fprintf(stderr, "%s", string);
}

MCRYPT session;
MCRYPT init_mcrypt_session(char *key_pathname);
void close_mcrypt_session(MCRYPT session);

int read_socket(int sockfd, void *buf, size_t size);
void write_socket(int sockfd, void const *buf, size_t size);

int child_pid;

void wait_child() {
  int status = 0;
  _c(waitpid(child_pid, &status, 0), "Failed to wait for child process");
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", WTERMSIG(status),
          WEXITSTATUS(status));
}

void exit_cleanup() {
  close_mcrypt_session(session);
  _c(close(sockfd), "Failed to close socket");
};

void usage();

struct termios orig_termios_attr;

void sigpipe_handler(int sig);

int serve(unsigned int portno);

int main(int argc, char *argv[]) {
  /* options descriptor */
  static struct option const long_opts[] = {
      {"port", required_argument, NULL, PORT_SHORT_OPTION},
      {"debug", no_argument, NULL, DEBUG_SHORT_OPTION},
      {"encrypt", required_argument, NULL, ENCRYPT_SHORT_OPTION},
      {0, 0, 0, 0},
  };

  unsigned int portno;
  char key_pathname[256];

  /* option parsing */
  int optc;
  while ((optc = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) {
    switch (optc) {
      case PORT_SHORT_OPTION:
        port_set = true;
        portno = atoi(optarg);
        break;
      case DEBUG_SHORT_OPTION:
        // print debug info
        debug = true;
        break;
      case ENCRYPT_SHORT_OPTION:
        encrypt_set = true;
        strcpy(key_pathname, optarg);
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

  if (encrypt_set) init_mcrypt_session(key_pathname);

  sockfd = serve(portno);

  atexit(exit_cleanup);

  char buf[BUF_SIZE];

  /* initializing pipes */
  int server_to_shell_fd[2], shell2server_fd[2];
  _c(pipe(server_to_shell_fd), "Failed to create terminal-to-shell pipe");
  _c(pipe(shell2server_fd), "Failed to create shell-to-terminal pipe");

  int *from_server_fd = &server_to_shell_fd[0],
      *to_shell_fd = &server_to_shell_fd[1];
  int *from_shell_fd = &shell2server_fd[0], *to_server_fd = &shell2server_fd[1];

  /* forking */
  _c(child_pid = fork(), "Failed to fork");

  bool const is_server = (child_pid > 0);
  bool const is_shell = (child_pid == 0);
  /* server process */
  if (is_server) {
    signal(SIGPIPE, sigpipe_handler);

    // no need to read from itself
    _c(close(*from_server_fd),
       "Failed to close the read end of terminal-to-shell pipe");
    // no need to write to itself
    _c(close(*to_server_fd),
       "Failed to close the write end of shell-to-terminal pipe");

    struct pollfd pollfds[2];
    pollfds[0].fd = sockfd;
    pollfds[0].events = POLLIN + POLLHUP + POLLERR;
    pollfds[1].fd = *from_shell_fd;
    pollfds[1].events = POLLIN + POLLHUP + POLLERR;

    ssize_t count;
    while (true) {
      _c(poll(pollfds, 2, -1), "Failed to poll stdin and from_shell");

      /* process socket inputs and forward them to the shell */
      if (pollfds[0].revents & POLLIN) {
        count = read_socket(sockfd, buf, sizeof(buf));
        if (count == 0) {
          wait_child();
          exit(EXIT_SUCCESS);
        }
        for (ssize_t i = 0; i < count; i++) switch (buf[i]) {
            case EOT:
              debug_write(EOT_REPR, 1);
              _c(close(*to_shell_fd),
                 "Failed to close the write end of terminal-to-shell pipe");
              // Should cause (pollfds[1].revents & POLLHUP) to be true
              break;
            case INTERRUPT:
              debug_write(INTERRUPT_REPR, 1);
              _c(kill(child_pid, SIGINT),
                 "Failed to send kill signal to child process");
              break;
            case '\r':
            case '\n':
              debug_write(CRLF, 2);
              _c(write(*to_shell_fd, &LF, 1), "Failed to write to shell");
              break;
            default:
              debug_write(&buf[i], 1);
              _c(write(*to_shell_fd, &buf[i], 1), "Failed to write to shell");
          }
      }

      /* process shell outputs and forward it to the socket */
      if (pollfds[1].revents & POLLIN) {
        _c(count = read(*from_shell_fd, buf, sizeof(buf)),
           "Failed to read from shell-to-terminal pipe");
        if (count == 0) {
          wait_child();
          exit(EXIT_SUCCESS);
        }
        for (ssize_t i = 0; i < count; i++) switch (buf[i]) {
            case '\n':
              debug_write(CRLF, 2);
              write_socket(sockfd, CRLF, 2);
              break;
            case EOT:
              debug_write(EOT_REPR, 2);
              write_socket(sockfd, &buf[i], 1);
              _c(close(*from_shell_fd),
                 "Failed to close the read end of shell-to-terminal pipe");
              wait_child();
              exit(EXIT_SUCCESS);
              break;
            default:
              debug_write(&buf[i], 1);
              write_socket(sockfd, &buf[i], 1);
          }
      }

      if (pollfds[0].revents & (POLLHUP | POLLERR)) {
        if (pollfds[0].revents & POLLERR)
          debug_printf("Socket has poll err.\r\n");
        if (pollfds[0].revents & POLLHUP)
          debug_printf("Socket has hung up.\r\n");
        _c(count = read(*from_shell_fd, buf, sizeof(buf)),
           "Failed to read from shell-to-terminal pipe");
        for (ssize_t i = 0; i < count; i++) switch (buf[i]) {
            case '\n':
              debug_write(CRLF, 2);
              _c(write(STDOUT_FILENO, CRLF, 2), "Failed to write to stdout");
              break;
            default:
              debug_write(&buf[i], 1);
              _c(write(STDOUT_FILENO, &buf[i], 1), "Failed to write to stdout");
          }
        wait_child();
        exit(EXIT_SUCCESS);
      }

      if (pollfds[1].revents & (POLLHUP | POLLERR)) {
        if (pollfds[1].revents & POLLERR)
          debug_printf("from_shell has poll err.\r\n");
        if (pollfds[1].revents & POLLHUP)
          debug_printf("from_shell has hung up.\r\n");
        _c(close(*from_shell_fd),
           "Failed to close the read end of shell-to-terminal pipe");
        wait_child();
        exit(EXIT_SUCCESS);
      }
    }
  }

  /* child (shell) process */
  if (is_shell) {
    // no need to read from itself
    _c(close(*from_shell_fd),
       "Failed to close the read end of shell-to-terminal pipe");
    // no need to write to itself
    _c(close(*to_shell_fd),
       "Failed to close the write end of terminal-to-shell pipe");

    // redirect the output of terminal-to-shell pipe to stdin
    _c(dup2(*from_server_fd, STDIN_FILENO),
       "Failed to redirect the read end of terminal-to-shell pipe to stdin");
    // close the original terminal-to-shell read end
    _c(close(*from_server_fd),
       "Failed to close the read end of terminal-to-shell pipe");

    // redirect stdout to the input of shell-to-terminal pipe
    _c(dup2(*to_server_fd, STDOUT_FILENO),
       "Failed to redirect the write end of shell-to-terminal pipe to "
       "stdout");
    // redirect stderr to the input of shell-to-terminal pipe
    _c(dup2(*to_server_fd, STDERR_FILENO),
       "Failed to redirect the write end of shell-to-terminal pipe to "
       "stderr");
    _c(close(*to_server_fd),
       "Failed to close the write end of shell-to-terminal pipe");

    _c(execlp(shell_program, shell_program, NULL),
       "Failed to execute the designated shell program");
  }

  wait_child();
  exit(EXIT_SUCCESS);
}

void usage() {
  // TODO: proper usage message
  exit(EXIT_FAILURE);
}

int serve(unsigned int portno) {
  struct sockaddr_in serv_addr, cli_addr;  // server_address, client_address
  unsigned int cli_len = sizeof(struct sockaddr_in);
  int listenfd, retfd = 0;
  _c(listenfd = socket(AF_INET, SOCK_STREAM, 0),
     "Failed to open listen socket");
  // clear serv_addr
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;  // IPv4 address family
  // server can accept connection from any client IP
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);  // setup port number
  _c(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)),
     "Failed to bind socket to port");  // bind socket to port
  // let the socket listen, maximum 5 pending connections
  _c(listen(listenfd, 5), "Failed to listen to socket");
  // wait for client’s connection, cli_addr stores client’s address
  _c((retfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len)),
     "Failed to accept client's connection");
  debug_printf("Client connected\n");

  return retfd;
}

void sigpipe_handler(int sig) {
  fprintf(stderr, "SIGPIPE received. signal: %d\r\n", sig);
  wait_child();
  exit(EXIT_SUCCESS);
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
  if (encrypt_set) mdecrypt_generic(session, buf, count);
  return count;
}

void write_socket(int sockfd, void const *buf, size_t size) {
  if (encrypt_set) {
    char buf_encrypted[256];
    memcpy(buf_encrypted, buf, size);
    mcrypt_generic(session, buf_encrypted, size);
    _c(send(sockfd, buf_encrypted, size, 0), "Failed to send to socket buffer");
  } else {
    _c(send(sockfd, buf, size, 0), "Failed to send to socket buffer");
  }
}
