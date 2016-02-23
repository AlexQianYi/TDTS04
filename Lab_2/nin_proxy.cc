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
#include <string>
#include <iostream>
#include <algorithm>

/*
 * 1. errno?
 * 2. waitpid?
 * 3. WNOHANG?
 * 4. struct sockaddr.sin_addr?
 * 5. sigaction?
 */
      
#define BACKLOG 10   // size of queue for pending connections
#define MAXDATASIZE 15000 // max number of bytes we can get at once
#define HUGEDATASIZE 8000000 // temporary measure
#define STDSERVPORT "80" // Webserver service port

using namespace std;
    
int do_child_stuff(int);

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
		   sizeof(yes)) == -1) {
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

      // The child shall not meddle with the parent
      close(sock_fd);
      do_child_stuff(new_fd);

      cout << "Closing connection on " << s << ':' << argv[1] << ".\n";
      close(new_fd);
      exit(0);
     
    }

    // The parent shall not meddle with the child
    close(new_fd);
  }

  return 0; 
}











int do_child_stuff(int clientproxy_fd)
{
  int proxyserver_fd, numbytes{}, numbytes2{};
  char buf[MAXDATASIZE], buf2[HUGEDATASIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];
  
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Receive HTTP message from client.
  if ((numbytes = recv(clientproxy_fd, buf, MAXDATASIZE-1, 0)) == -1) {
    perror("Child: recv from client");
    return 1;
  }

  cout << "GET request from browser client: \n" << string(buf, numbytes) << "\n"; // DEBUGGING LINE
  
  if ( numbytes == 0 ) {
    perror("number of bytes received from browser was 0");
    return 2;
  }
  
  const string httpreq (buf, numbytes);
  
  // Filter GET line.

  size_t pos { httpreq.find("\r\n") };
  string temp { httpreq.substr(0, pos) };

  transform(temp.begin(), temp.end(), temp.begin(), ::tolower);
  
  //  if( temp. 
  
  // Determine the Host
  
  cout << "This is the httpreq string:\n" << httpreq << "\n"; // DEBUGGING LINE
  pos = httpreq.find("\r\nHost: ");
  const char *csaddress;

  if (pos == string::npos) {
    perror("no Host: header in HTTP message");
    return 3;
  }

  pos = pos + 8;
  size_t lastpos { httpreq.find("\r\n", pos) };

  cout << "Pos: " << pos << "   Lastpos: " << lastpos << "\n"; // DEBUGGING LINE

  temp = httpreq.substr(pos, lastpos-pos);
  csaddress = temp.c_str();
  cout << "(DEBUG) Host:.::" << csaddress << "::.\n";


  

  // Get the address info. Port 80 for webserver.
  if ((rv = getaddrinfo(csaddress, STDSERVPORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "Child: getaddrinfo(): %s\n", gai_strerror(rv));
    return 4;
  }
  
  // Change Connection-header value to close
  //               0 1 23456789*123
  pos = httpreq.find("\r\nConnection: ");

  if (pos == string::npos) {
    printf("Child: could not find a connection header in the GET request.\n");
  }
  else {
    pos = pos + 14;
    lastpos = httpreq.find("\r\n", pos);
    cout << "(DEBUG) raw::Connection:.::" << httpreq.substr(pos, lastpos-pos) << "::.\n";
    memmove(buf+pos+5, buf+lastpos, numbytes-lastpos);
    memcpy(buf+pos, "close", 5); // strcpy would invite its unpopular friend, Slash-Zero.
    numbytes = numbytes - (lastpos-pos) + 5;
  }

  // Loop through servinfo and connect
  for (p = servinfo ; p != nullptr ; p = p->ai_next) {
    
    if ((proxyserver_fd = socket(p->ai_family,
				 p->ai_socktype,
				 p->ai_protocol)) == -1) {
      perror("Child: socket()");
      continue;
    }
	
    if (connect(proxyserver_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(proxyserver_fd);
      perror("Child: connect()");
      continue;
    }
	
    break;
  }

  if (p == nullptr) {
    fprintf(stderr, "Child: failed to connect to server\n");
    return 5;
  }
  
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
	    s, sizeof s);
  freeaddrinfo(servinfo);
  cout << "Proxy has connected to " << s << " on port " << STDSERVPORT << ".\n";

  // Forward our message
  if (send(proxyserver_fd, buf, numbytes, 0) == -1) {
    perror("Child: send to server");
    return 6;
  }

  buf[numbytes] = '\0';

  cout << "Sent message:\n" << buf << "\n";

  // SOME DEBUGGING
  if (buf[numbytes] == '\0')
    cout << "buf[numbytes]: is a \\0\n";
  if (buf[numbytes-1] == '\n')
    cout << "buf[numbytes-1]: is a \\n\n";
  if (buf[numbytes-2] == '\r')
    cout << "buf[numbytes-2]: is a \\r\n";
  if (buf[numbytes-3] == '\n')
    cout << "buf[numbytes-3]: is a \\n\n";
  if (buf[numbytes-4] == '\r')
    cout << "buf[numbytes-4]: is a \\r\n";

  cout << buf+numbytes-10 << "This row.\n";
  // /SOME DEBUGGING

  
  // Receive response from server
 int whirrwhirr{}; 
  
  while ((numbytes = recv(proxyserver_fd, buf, MAXDATASIZE-1, 0)) != -1
	 && numbytes != 0) {
    memcpy(buf2+numbytes2, buf, numbytes);
    numbytes2 += numbytes;
    ++whirrwhirr;
    cout << string(buf, numbytes) << "\n"; // DEBUGGING LINE
  }
  
  if (numbytes != 0) {
    perror("Child: recieve server response\n");
    return 7;
  }
  else if (numbytes2 == 0) {
    printf("Received no TCP data.\n");
      }
  else {
    cout << "Received response:\n" << string(buf2, numbytes2) << "\n ... in "
	 << whirrwhirr << " receives of total " << numbytes2 << " bytes.\n";
  }

  
  // Forward response to client  
  if (send(clientproxy_fd, buf2, numbytes2, 0) == -1) {
    perror("Child: send back to client");
    return 8;
  }


  

  cout << "Closing connection on " << s << ':' << STDSERVPORT << ".\n";
  close(proxyserver_fd);

  return 0;
}
