#pragma once
#include "swrappers.hh"
#include <boost/utility/string_ref.hpp>

int SConnectWithTimeout(int sockfd, const ComboAddress& remote, double timeout);

struct RAIISocket
{
  RAIISocket(int fd) : d_fd(fd){}
  RAIISocket(int domain, int type, int protocol=0)
  {
    d_fd = SSocket(domain, type, protocol);
  }

  operator int()
  {
    return d_fd;
  }
  
  void release()
  {
    d_fd=-1;
  }
  ~RAIISocket()
  {
    if(d_fd >= 0)
      close(d_fd);
  }
  int d_fd;
};

// ignores timeout for now
class ReadBuffer
{
public:
  explicit ReadBuffer(int fd, int bufsize=1024) : d_fd(fd), d_buffer(bufsize)
  {}

  void setTimeout(double timeout)
  {
    d_timeout = timeout;
  }
  
  inline bool getChar(char* c)
  {
    if(d_pos == d_endpos && !getMoreData())
      return false;
    *c=d_buffer[d_pos++];
    return true;
  }
  
private:
  bool getMoreData(); //!< returns false on EOF
  int d_fd;
  std::vector<char> d_buffer;
  unsigned int d_pos{0};
  unsigned int d_endpos{0};
  double d_timeout=-1;
};

// convenience class that requires a non-blocking socket as input and supports commonly used operations
class SocketCommunicator
{
public:
  explicit SocketCommunicator(int fd) : d_rb(fd), d_fd(fd)
  {
    SetNonBlocking(fd);
  }
  void connect(const ComboAddress& a)
  {
    SConnectWithTimeout(d_fd, a, d_timeout);
  }
  bool getLine(std::string& line);
  void writen(boost::string_ref message);
  void setTimeout(double timeout) { d_timeout = timeout; }
private:
  ReadBuffer d_rb;
  int d_fd;
  double d_timeout{-1};
};
