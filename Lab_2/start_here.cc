#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>

/*
 * 1. errno?
 * 2. waitpid?
 * 3. WNOHANG?
 * 4. struct sockaddr.sin_addr?
 * 5. sigaction?
 * 6. Hitta hur vi friar upp bundna sockets/portar (i handledningen el. Beej's)
 */
      
#define BACKLOG 10   // size of queue for pending connections
#define MAXDATASIZE 1514 // max number of bytes we can get at once

using namespace std;
    

void sigchld_handler(int s)
{
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;
    
  while(waitpid(-1, NULL, WNOHANG) > 0);
    
  errno = saved_errno;
}  
    
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
    
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
   

/* ************
 * *** MAIN ***
 * ***********/
int main(int argc, char* argv[])
{
  if (argc != 2) {
    fprintf(stderr,"Correct command to start program: NetNinny <port number>\n");
    return 1;
  }
  
  int argv_1 { stoi(argv[1]) };

  if (argv_1 < 1025 || argv_1 > 65535) {
    fprintf(stderr,"Specify ONE argument for a port the proxy will listen to. (Hint: any unused port between 1025 and 65535.)\n");
    return 2;
  }

  int sock_fd, new_fd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv; // "Return Value"

  int numbytes;
  char buf[MAXDATASIZE];

  /*
   * Setting up the listening socket
   */
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  // getaddrinfo()
  if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 3;
  }

  // loop through results and bind
  for (p = servinfo ; p != nullptr ; p = p->ai_next) {
    if ((sock_fd = socket(p->ai_family,
			  p->ai_socktype,
			  p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
		   sizeof(int)) == -1) {
      perror("setsocketopt");
      exit(1);
    }

    if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sock_fd);
      perror("server: bind");
      continue;
    }
    
    break;
  }
  
  freeaddrinfo(servinfo); 
    
  if (p == nullptr) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }
    
  if (listen(sock_fd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }   
  /*
   *  /Listening Socket setup
   */


  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
  

  cout << "Proxy is listening on port " << argv_1 << ".\n";
  printf("Awaiting connections...\n");
  

  /*
   * Listening loop
   */
  while(1) {
    // Listen for connections
    sin_size = sizeof their_addr;
    new_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    // Report to command window
    inet_ntop(their_addr.ss_family,
	      get_in_addr((struct sockaddr *)&their_addr),
	      s, sizeof s);
    printf("server: got connection from %s\n", s);

    // Fork it - child domain
    if (!fork()) {
      // while(1) { // *** loop-condition TBD
	// The child shall not meddle with the parent
	close(sock_fd);

	if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
	  perror("recv");
	  exit(1);
	}

	cout << "Number of bytes received: " << numbytes << "\n";

	buf[numbytes] = '\0';

	printf("Received from webclient:\n'%s'\n", buf);

	// LEAVE THE WEBCLIENT HANGING YO

	if(send(new_fd, "Successfully received message.", 30, 0) == -1)
	  perror("send");

	close(new_fd);
	exit(0);


	/*	if (send(new_fd, "Hello, world!", 13, 0) == -1)
		perror("send"); */
	//     }
    }
    // The parent shall not meddle with the child
    close(new_fd);
  }

  return 0; 
}

