#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "8080"
#define MSG "Hello, World!"
#define BACKLOG 10
#define EXIT_ERROR -1

void sigchld_handler(int s) {
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
  int res, sockfd, cfd;
  struct addrinfo hints, *serverinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size = sizeof(struct sockaddr_storage);
  struct sigaction sa;
  char s[INET6_ADDRSTRLEN];
  int msglen = strlen(MSG);
  int yes = 1;

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
      perror("socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(EXIT_ERROR);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("bind");
      continue;
    }

    break;
  }

  freeaddrinfo(serverinfo);

  if (p == NULL) {
    printf("Error: server unable to bind\n");
    exit(EXIT_ERROR);
  }

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(EXIT_ERROR);
  }

  // reap dead processes
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_ERROR);
  }

  printf("Server listening on port: %s\n", PORT);

  while (1) {
    cfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (cfd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(cfd, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s));

    printf("Server: got connection from %s\n", s);

    if (!fork()) {
      close(sockfd);

      if (send(cfd, MSG, msglen, 0) == -1) {
        perror("send");
      }

      close(cfd);
      exit(0);
    }

    close(cfd);
  }

  return 0;
}
