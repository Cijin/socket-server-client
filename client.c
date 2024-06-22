#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "8080"
#define BUF_SIZE 4096
#define EXIT_ERROR -1

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
  int res, sockfd, recievebytes;
  struct addrinfo hints, *serverinfo, *p;
  char s[INET6_ADDRSTRLEN], buf[BUF_SIZE];

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((res = getaddrinfo(NULL, PORT, &hints, &serverinfo)) != 0) {
    printf("getaddrinfo error: %s\n", gai_strerror(res));
    exit(EXIT_ERROR);
  }

  for (p = serverinfo; p != NULL; p = p->ai_next) {
    if (((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))) ==
        -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("client: connect");
      continue;
    }

    break;
  }

  if (p == NULL) {
    printf("Error: client unable to connect\n");
    exit(EXIT_ERROR);
  }

  inet_ntop(sockfd, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));

  printf("Client: connecting to %s\n", s);

  freeaddrinfo(serverinfo);

  if ((recievebytes = recv(sockfd, buf, BUF_SIZE - 1, 0)) == -1) {
    perror("recv");
    exit(EXIT_ERROR);
  }

  buf[recievebytes] = '\0';

  printf("Client recieved: %s\n", buf);

  close(sockfd);

  return 0;
}
