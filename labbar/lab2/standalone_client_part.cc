#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>

#define MAXDATASIZE 100 // max number of bytes we can get at once

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{

  if (argc != 3) {
    fprintf(stderr,"Correct command to start test client: placeholder <address> <port number>\n");
    return 1;
  }

  int argv_2 { stoi(argv[2]) };

  if (argv_2 < 1025 || argv_2 > 65535) {
    fprintf(stderr,"Specify ONE argument for the serverport the client will try to connect to. (Hint: Between 1025 and 65535.)\n");
    return 2;
  }

  int sock_fd, numbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p;    
  int rv;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Get the address info, duh. Stored as a linked list with pointer servinfo pointing to head.
  if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 3;  
  }

  // loop through all the results and connect
  for (p = servinfo ; p != nullptr ; p = p->ai_next) {
    
    if ((sock_fd = socket(p->ai_family,
			  p->ai_socktype,
			  p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sock_fd);
      perror("client: connect");
      continue;
    }

    break;
  }
  
  if(p == nullptr) {
    fprintf(stderr, "client: failed to connect\n");
    return 4;
  }
  
  if (send(sock_fd, "This is a test message!", 22, 0) == -1)
    perror("send");

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
	    s, sizeof s);
  printf("client: connecting to %s\n", s);

  freeaddrinfo(servinfo); // all done with this structure

  if ((numbytes = recv(sock_fd, buf, MAXDATASIZE-1, 0)) == -1) {
    perror("recv");
    exit(1);
  }

  buf[numbytes] = '\0';

  printf("client: received '%s'\n", buf);

  close(sock_fd);

  return 0;
}
