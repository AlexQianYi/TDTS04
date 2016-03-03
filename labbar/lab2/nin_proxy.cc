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

/*** CODE STRUCTURE: ***
 *
 * help function declarations
 *
 * INT MAIN *
 * - <7> Command argument
 * - <7> Setup of listening socket
 * --- getaddrinfo()
 * --- loop through results and bind
 * - Sigaction settings
 * - Listening loop
 * --- <2> Listen, report and fork children -> do_child_stuff()
 *
 * do_child_stuff() *
 * - <2> Receive HTTP message from client 
 * - <2> Determine the host 
 * - <3> FILTER the request 
 * - <6> Change the GET/POST line
 * - <2> Modify Connection-header
 * - <2> Get address info of server 
 * - <2> Loop through results and connect 
 * - <2> Forward request to server 
 * - (Hopefully) receive response from server  [RCV]
 * - <8> FILTER the response
 * - Forward the response to client
 * - End child session
 */

/*** Notes about shortcomings of the proxy: ***
 * 1. Currently, our proxy tries to receive all the data associated with the response to one GET-request before forwarding it back to the client.
 * - 1.1. We only really need to work against a large buffer when we are to filter. Our current implementation ([RCV]) of the receive-loop should be split in two parts:
     - Receive until header end "\r\n\r\n".
     - Look for Content-Type: text and the lack of Content-Encoding: gzip
     - If, collect the whole response in a buffer and filter. If not, enter a different send-receive-loop that sends as we receive.
 * 2. The proxy does not modify the Accept-Encoding header.
   - This leads to receiving some text content compressed, obstructing the proxy's ability to filter.
 * 3. The proxy is not optimized for receiving Chrome/Chromium GET-requests.
   - This problem manifests itself in how the proxy handles the Connection header. It only tries to change the value of the connection header to close, rather than adding one in the absence of one.
   - This means browsing with the proxy in Chrome/Chromium results in the child having to wait for timeouts before forwarding all (due to our current implementation) server responses.
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
  if (!(argc == 2)) {
    fprintf(stderr,"Correct command to start program: ninnyproxy <port number>\n");
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
  size_t pos, lastpos;
  string response302 {"HTTP/1.1 302 Found\r\nLocation: http://www.ida.liu.se/~TDTS04/labs/2011/ass2/error1.html\r\n\r\n"};
  const size_t r302_ienr {80};
  
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;


  // Receive HTTP message from client.
  string httpreq;
  
  if ((numbytes = recv(clientproxy_fd, buf, MAXDATASIZE-1, 0)) == -1) {
    perror("Child: recv from client");
    return 1;
  }
  
  if ( numbytes == 0 ) {
    perror("number of bytes received from browser was 0");
    return 2;
  }
  
  httpreq = string(buf, numbytes);
  //cout << "GET request from browser client: \n" << httpreq << "\n"; // DEBUGGING LINE
  

  // Determine the Host
  string shostaddress;  
  
  pos = httpreq.find("\r\nHost: ");
  
  if (pos == string::npos) {
    perror("no Host: header in HTTP message");
    return 3;
  }
  
  pos = pos + 8;
  lastpos = httpreq.find("\r\n", pos); 
  // cout << "Pos: " << pos << "   Lastpos: " << lastpos << "\n"; // DEBUGGING LINE 
  shostaddress = httpreq.substr(pos, lastpos-pos);
  // cout << "(DEBUG) Host:.::" << shostaddress << "::.\n"; // DEBUGGING LINE

  
  // Filter the request.
  pos = httpreq.find("\r\n");
  string filterline { shostaddress + httpreq.substr(0, pos) };

  transform(filterline.begin(), filterline.end(), filterline.begin(), ::tolower);
  filterline.erase(remove_if(filterline.begin(), filterline.end(), [](char a)->bool{ return a == '+'; }),
	     filterline.end()); // remove <space>
  pos = 0;
  while ((pos = filterline.find("%20", pos)) != string::npos) { // remove <space>
    filterline.erase(pos, 3);
  }
  pos = 0;
  while ((pos = filterline.find("%c3%b6", pos)) != string::npos) { // ö - mind the tolower function
    filterline.replace(pos, 6, "o");
  }
  pos = 0;
  while ((pos = filterline.find("%c3%96", pos)) != string::npos) { // Ö - mind the tolower function
    filterline.replace(pos, 6, "o");
  }
  
  if (filterline.find("spongebob") != string::npos ||
      filterline.find("britneyspears") != string::npos ||
      filterline.find("parishilton") != string::npos ||
      filterline.find("norrkoping") != string::npos) {

    response302.at(r302_ienr) = '1';

    if (send(clientproxy_fd, response302.data(), response302.length(), 0) == -1) {
      perror("Child: 302 send back to client");
      return 4;
    }
    else {
      return 0;
    }
  }
  

  // Change the GET/POST line path to not include the host name
  if ((lastpos = httpreq.find("HTTP/")) == string::npos) {
    perror("Child: no HTTP version in the client request");
    return 5;
  }

  if ((pos = httpreq.substr(0, lastpos).find("http://")) != string::npos) {
    lastpos = httpreq.find('/', pos+7);
    numbytes = numbytes - (lastpos - pos);
    httpreq.erase(pos, lastpos - pos);
  }

  
  // Change Connection-header value to close
  //                       0 1 23456789*123
  if ((pos = httpreq.find("\r\nConnection: ")) == string::npos) {
    printf("Child: could not find a connection header in the GET request.\n");
    // Here should be code that implements adding a Connection: Close header"
  }
  else {
    pos = pos + 14;
    lastpos = httpreq.find("\r\n", pos);
    numbytes = numbytes - (lastpos-pos) + 5;
    //cout << "(DEBUG) raw::Connection:.::" << httpreq.substr(pos, lastpos-pos) << "::.\n";
    httpreq.replace(pos, lastpos - pos, "close");
  }

  // Used for making sure numbytes variable was updated correctly. Could be left in.
  /*if (numbytes != httpreq.length()) { 
    cout << "WARNING WARNING" << "\n";
    exit(1);
    }*/

  //cout << "Modified GET request:\n" << httpreq << "\n"; // DEBUGGING LINE


  // Get the address info. Port 80 for webserver.
  if ((rv = getaddrinfo(shostaddress.data(), STDSERVPORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "Child: getaddrinfo(): %s\n", gai_strerror(rv));
    return 6;
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
    return 7;
  }
  
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
	    s, sizeof s);
  freeaddrinfo(servinfo);
  //cout << "Proxy has connected to " << s << " on port " << STDSERVPORT << ".\n"; // DEBUGGING LINE


  // Forward our message
  if (send(proxyserver_fd, &(httpreq.front()), numbytes, 0) == -1) {
    perror("Child: send to server");
    return 8;
  }
  

  // Receive response from server [RCV]
  int whirrwhirr{}; 
  
  while ((numbytes = recv(proxyserver_fd, buf, MAXDATASIZE-1, 0)) != -1
	 && numbytes != 0) {
    if (numbytes2 + numbytes >= HUGEDATASIZE) {
      break;
    }
    memcpy(buf2+numbytes2, buf, numbytes);
    numbytes2 += numbytes;
    ++whirrwhirr;
    // cout << string(buf, numbytes) << "\n"; // DEBUGGING LINE
  }
  
  if (numbytes != 0) {
    perror("Child: recieve server response\n");
    return 9;
  }
  else if (numbytes2 == 0) {
    printf("Received no TCP data.\n");
    return 10;
  }
  else {
    //cout << "Received response:\n" << string(buf2, numbytes2) << "\n ... in " // DEBUGGING LINE
    //	 << whirrwhirr << " receives of total " << numbytes2 << " bytes.\n"; // DEBUGGING LINE
  }


  // Filter response
  string sresponse (buf2, numbytes2);
  string sresponseheader (sresponse, 0, sresponse.find("\r\n\r\n"));

  //cout << "Response header:\n" << sresponseheader << "\n"; // DEBUGGING LINE

  if (sresponseheader.find("Content-Type: text") != string::npos &&
      sresponseheader.find("Content-Encoding: gzip") == string::npos) {
    
    if (sresponse.find("SpongeBob") != string::npos ||
	sresponse.find("Britney Spears") != string::npos ||
	sresponse.find("Paris Hilton") != string::npos ||
	sresponse.find("Norrköping") != string::npos) {
      response302.at(r302_ienr) = '2';

      if (send(clientproxy_fd, response302.data(), response302.length(), 0) == -1) {
	perror("Child: 302 send back to client");
	return 11;
      }
      else {
	return 0;
      }
    }
  }
    

  // Forward response to client  
  if (send(clientproxy_fd, buf2, numbytes2, 0) == -1) {
    perror("Child: send back to client");
    return 12;
  }


  // Done with child session
  cout << "Closing connection on " << s << ':' << STDSERVPORT << ".\n";
  close(proxyserver_fd);

  return 0;
}
