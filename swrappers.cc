#include "swrappers.hh"
#include <boost/format.hpp>
#include <unistd.h>
#include <fcntl.h>

/** these functions provide a very lightweight wrapper to the Berkeley sockets API. Errors -> exceptions! */

static void RuntimeError(const boost::format& fmt)
{
  throw std::runtime_error(fmt.str());
}


int SSocket(int family, int type, int flags)
{
  int ret = socket(family, type, flags);
  if(ret < 0)
    RuntimeError(boost::format("creating socket of type %d: %s") % family % strerror(errno));
  return ret;
}

int SConnect(int sockfd, const ComboAddress& remote)
{
  int ret = connect(sockfd, (struct sockaddr*)&remote, remote.getSocklen());
  if(ret < 0) {
    int savederrno = errno;
    RuntimeError(boost::format("connecting socket to %s: %s") % remote.toStringWithPort() % strerror(savederrno));
  }
  return ret;
}

int SBind(int sockfd, const ComboAddress& local)
{
  int ret = bind(sockfd, (struct sockaddr*)&local, local.getSocklen());
  if(ret < 0) {
    int savederrno = errno;
    RuntimeError(boost::format("binding socket to %s: %s") % local.toStringWithPort() % strerror(savederrno));
  }
  return ret;
}

int SAccept(int sockfd, ComboAddress& remote)
{
  socklen_t remlen = remote.getSocklen();

  int ret = accept(sockfd, (struct sockaddr*)&remote, &remlen);
  if(ret < 0)
    RuntimeError(boost::format("accepting new connection on socket: %s") % strerror(errno));
  return ret;
}

int SListen(int sockfd, int limit)
{
  int ret = listen(sockfd, limit);
  if(ret < 0)
    RuntimeError(boost::format("setting socket to listen: %s") % strerror(errno));
  return ret;
}

int SSetsockopt(int sockfd, int level, int opname, int value)
{
  int ret = setsockopt(sockfd, level, opname, &value, sizeof(value));
  if(ret < 0)
    RuntimeError(boost::format("setsockopt for level %d and opname %d to %d failed: %s") % level % opname % value % strerror(errno));
  return ret;
}

void SWrite(int sockfd, boost::string_ref content, std::string::size_type *wrlen)
{
  int res = write(sockfd, &content[0], content.size());
  if(res < 0)
    RuntimeError(boost::format("Write to socket: %s") % strerror(errno));
  if(res != (int)content.size()) {
    if(wrlen) {
      *wrlen = res;
      return;
    }
    RuntimeError(boost::format("Partial write to socket: wrote %d bytes out of %d") % res % content.size());
  }
}

std::string SRead(int sockfd, std::string::size_type limit)
{
  std::string ret;
  char buffer[1024];
  std::string::size_type leftToRead=limit;
  for(;;) {
    auto chunk = sizeof(buffer) < leftToRead ? sizeof(buffer) : leftToRead;
    int res = read(sockfd, buffer, chunk);
    if(res < 0)
      RuntimeError(boost::format("Read from socket: %s") % strerror(errno));
    if(!res)
      break;
    ret.append(buffer, res);
  }
  return ret;
}

void SetNonBlocking(int sock, bool to)
{
  int flags=fcntl(sock,F_GETFL,0);
  if(flags<0)
    RuntimeError(boost::format("Retrieving socket flags: %s") % strerror(errno));

  // so we could optimize to not do it if nonblocking already set, but that would be.. semantics
  if(to) {
    flags |= O_NONBLOCK;
  }
  else 
    flags &= (~O_NONBLOCK);
      
  if(fcntl(sock, F_SETFL, flags) < 0)
    RuntimeError(boost::format("Setting socket flags: %s") % strerror(errno));
}

std::vector<ComboAddress> resolveName(boost::string_ref name, bool ipv4, bool ipv6)
{
  std::vector<ComboAddress> ret;

  for(int n = 0; n < 2; ++n) {
    struct addrinfo* res;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = n ? AF_INET : AF_INET6;
    if(hints.ai_family == AF_INET && !ipv4)
      continue;
    if(hints.ai_family == AF_INET6 && !ipv6)
      continue;
    
    ComboAddress remote;
    remote.sin4.sin_family = AF_INET6;
    if(!getaddrinfo(&name[0], 0, &hints, &res)) { // this is how the getaddrinfo return code works
      struct addrinfo* address = res;
      do {
        if (address->ai_addrlen <= sizeof(remote)) {
          memcpy(&remote, address->ai_addr, address->ai_addrlen);
          remote.sin4.sin_port=0;
          ret.push_back(remote);
        }
      } while((address = address->ai_next));
      freeaddrinfo(res);
    }
  }
  return ret;
}

