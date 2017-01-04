#pragma once
#include "swrappers.hh"
#include <boost/utility/string_ref.hpp>

// ignores timeout for now
class ReadBuffer
{
public:
  explicit ReadBuffer(int fd, int bufsize=1024) : d_fd(fd), d_buffer(bufsize)
  {}

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
};

// convenience class that requires a non-blocking socket as input and supports commonly used operations
class SocketCommunicator
{
public:
  explicit SocketCommunicator(int fd) : d_rb(fd), d_fd(fd) {}
  bool getLine(std::string& line);
  void writen(boost::string_ref message);
private:
  ReadBuffer d_rb;
  int d_fd;
};
