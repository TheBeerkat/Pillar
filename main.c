#include <arpa/inet.h>
#include <err.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define LISTEN_BACKLOG 50
#define SOCKET_TIMEOUT_SECONDS 5
#define SOCKET_TIMEOUT_MICROSECONDS 0

int verify_addr(char *addr) {
  // TODO: verify address with regex
  if (strlen(addr) < 7)
    return 1;
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Must specify IP interface address.\n");
    return -1;
  }
  if (verify_addr(argv[1])) {
    fprintf(stderr, "IP interface address verification failed\n");
    return -1;
  }

  // Initialize socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    err(EXIT_FAILURE, "socket failed to provide a file descriptor");
  }
  // This is a crude solution to slowloris attacks.
  // TODO: implement multiplexing (epoll). only write when client is ready to
  // read data, thus avoiding write block. Same for read.
  struct timeval tv;
  tv.tv_sec = SOCKET_TIMEOUT_SECONDS;
  tv.tv_usec = SOCKET_TIMEOUT_MICROSECONDS;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
  setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));

  // Initialize IP interface address
  struct in_addr addr;
  inet_aton(argv[1], &addr);

  // Initialize IP socket address (IP interface address + 16-bit port number)
  struct sockaddr_in sock_addr;
  memset(&sock_addr, 0, sizeof(sock_addr)); // zero padded struct

  sock_addr.sin_family = AF_INET;
  sock_addr.sin_port = htons(atoi(argv[2]));
  sock_addr.sin_addr = addr;

  // Assigning name to address i.e. binding
  int binding =
      bind(sockfd, (const struct sockaddr *)&sock_addr, sizeof(sock_addr));
  if (binding == -1) {
    err(EXIT_FAILURE, "bind failed");
  }

  // Listen on socket address
  int listened = listen(sockfd, LISTEN_BACKLOG);
  if (listened == -1) {
    err(EXIT_FAILURE, "listen failed");
  }

  // Accept connections
  while (1) {

    struct sockaddr peer_sock_addr;
    memset(&peer_sock_addr, 0, sizeof(peer_sock_addr));

    int peer_sock_len = sizeof(sock_addr);
    int accepted_sockfd =
        accept(sockfd, &peer_sock_addr, (socklen_t *)&peer_sock_len);
    if (accepted_sockfd == -1) {
      warn("accept failed");
      continue;
    }

    char readbuffer[1025];
    readbuffer[1024] = '\0';
    int nbytes = read(accepted_sockfd, readbuffer, 2);
    if (nbytes == -1) {
      warn("read failed");
      break;
    }

    char *message = "HTTP/1.1 200 OK\r\n\r\nPillar Online\n";
    int sent = write(accepted_sockfd, message, strlen(message));
    if (sent == -1) {
      // TODO: on failure store bytes in a queue
      warn("send failed");
      continue;
    }

    // Cleanup
    int shutdown_accepted = shutdown(accepted_sockfd, SHUT_RDWR);
    if (shutdown_accepted == -1) {
      warn("shutdown failed");
      continue;
    }
    int closed_accepted = close(accepted_sockfd);
    if (closed_accepted == -1) {
      warn("close failed");
      continue;
    }
  }

  int shutdown_listening = shutdown(sockfd, SHUT_RDWR);
  if (shutdown_listening == -1) {
    err(EXIT_FAILURE, "shutdown failed");
  }

  int closed_listening = close(sockfd);
  if (closed_listening == -1) {
    err(EXIT_FAILURE, "close failed");
  }

  return 0;
}
