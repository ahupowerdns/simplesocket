#pragma once
#include <boost/utility/string_ref.hpp>

#include "comboaddress.hh"

/** \mainpage Simple Sockets Intro
    \section intro_sec Introduction
    From C++ the full POSIX and OS socket functions are available. These are very powerful, offer well known semantics, but are somewhat of a pain to use.

Various libraries have attempted to offer "C++ native" socket environments, but most of these offer different semantics that are less well known, or frankly, obvious. 

This set of files offers the BSD/POSIX socket API relatively unadorned, with the original semantics, and fully interoperable
with the usual system calls. 

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

    \section higher Some sample code using the higher-level classes
\code{.cpp}
  auto addresses=resolveName("ds9a.nl"); // this retrieves IPv4 and IPv6

  for(auto& a : addresses) {
    a.setPort(80);
    cout << "Connecting to: " << a.toStringWithPort() << endl;

    RAIISocket rs(a.sin.sin_family, SOCK_STREAM);
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
void SWrite(int sockfd, boost::string_ref content, std::string::size_type* wrlen=0);

std::string SRead(int sockfd, std::string::size_type limit = std::numeric_limits<std::string::size_type>::max());

//! Set a socket to (non) blocking mode. Error = exception.
void SetNonBlocking(int sockfd, bool to=true);

//! Use system facilities to resolve a name into addresses. If no address found, returns empty vector
std::vector<ComboAddress> resolveName(boost::string_ref name, bool ipv4=true, bool ipv6=true);

