#pragma once
#include <sys/poll.h>
#include "comboaddress.hh"
#include <map>
#include <vector>
#include <limits>

/** \mainpage Simple Sockets Intro
    \section intro_sec Introduction
    From C++ the full POSIX and OS socket functions are available. These are very powerful, offer well known semantics, but are somewhat of a pain to use.

Various libraries have attempted to offer "C++ native" socket environments, but most of these offer different semantics that are less well known, or frankly, obvious. 

This set of files offers the BSD/POSIX socket API relatively unadorned, with the original semantics, and fully interoperable
with the usual system calls. 

To find out more about Simple Sockets, please also consult its [GitHub](https://github.com/ahuPowerDNS/simplesocket) page.

    \section Basic Basic example code
Note that the 'raw' socket code below is just as reliable as the original socket API. In other words, it is recommended to use
higher level functions. 
\code{.cpp}
    #include "swrappers.hh"

    int main()
    {
      int s = SSocket(AF_INET, SOCK_STREAM);
      ComboAddress webserver("52.48.64.3", 80);
      SConnect(s, webserver);
      SWrite(s, "GET / HTTP/1.1\r\nHost: ds9a.nl\r\nConnection: Close\r\n\r\n"); // will throw on partial write
      std::string response = SRead(s); // will read the entire universe into RAM
      cout<< response << endl;
      close(s);
    }
\endcode

    \section Medium Slightly higher level code
This sample code uses some functions that are not just wrappers, and offer more useful semantics:
\code{.cpp}
    #include "swrappers.hh"

    int main()
    {
      int s = SSocket(AF_INET, SOCK_STREAM);
      SConnect(s, "52.48.64.3:80"_ipv4);     // user literal! with constexpr!
      SWriten(s, "GET / HTTP/1.1\r\nHost: ds9a.nl\r\nConnection: Close\r\n\r\n"); // deals with partial writes
      std::string response = SRead(s, 100000);  // reads at most 100k
      cout<< response << endl;
      close(s);
    }
\endcode

    \section higher Some sample code using the higher-level classes
\code{.cpp}
  auto addresses=resolveName("ds9a.nl"); // this retrieves IPv4 and IPv6

  for(auto& a : addresses) {
    a.setPort(80);
    cout << "Connecting to: " << a.toStringWithPort() << endl;

    Socket rs(a.sin.sin_family, SOCK_STREAM);
    SocketCommunicator sc(rs);
    sc.connect(a);
    sc.writen("GET / HTTP/1.1\r\nHost: ds9a.nl\r\nConnection: Close\r\n\r\n");

    std::string line;
    while(sc.getLine(line)) {
      cout<<"Got: "<<line;
    }
  }
\endcode

*/

/** \file swrappers.hh 
    \brief Set of C++ friendly wrappers around socket-relevant calls
*/

//! Create a socket. Error = exception.
int SSocket(int family, int type, int flags=0);
//! Connect a socket. Error = exception.
int SConnect(int sockfd, const ComboAddress& remote);
//! Bind a socket. Error = exception.
int SBind(int sockfd, const ComboAddress& local);

//! Accept a new connection on a socket. Error = exception.
int SAccept(int sockfd, ComboAddress& remote);

//! Enable listen on a socket. Error = exception.
int SListen(int sockfd, int limit);

//! Set a socket option. Error = exception.
int SSetsockopt(int sockfd, int level, int opname, int value);

//! Attempt to write string to the socket. Partial write is an exception if 'wrlen' not set. Otherwise wrlen gets size of written data, and no exception
void SWrite(int sockfd, const std::string& content, std::string::size_type* wrlen=0);

//! Attempt to write whole string to the socket, dealing with partial writes. EOF is exception.
void SWriten(int sockfd, const std::string& content);

//! Send a datagram to a destination
void SSendto(int sockfd, const std::string& content, const ComboAddress& dest, int flags=0);

//! Send a datagram to a connected socket
int SSend(int sockfd, const std::string& content, int flags=0);

//! Receive a datagram from a destination
std::string SRecvfrom(int sockfd, std::string::size_type limit, ComboAddress& dest, int flags=0);


//! Retrieve sockname
void SGetsockname(int sockfd, ComboAddress& dest);

//! Read at most \p bytes bytes from fd \p sockfd. Will stop reading after EOF, which is not an exception.
std::string SRead(int sockfd, std::string::size_type limit = std::numeric_limits<std::string::size_type>::max());

//! Set a socket to (non) blocking mode. Error = exception.
void SetNonBlocking(int sockfd, bool to=true);


std::map<int,short> SPoll(const std::vector<int>&rdfds, const std::vector<int>&wrfds, double timeout);

//! Use system facilities to resolve a name into addresses. If no address found, returns empty vector
std::vector<ComboAddress> resolveName(const std::string& name, bool ipv4=true, bool ipv6=true);

