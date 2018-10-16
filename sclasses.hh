#pragma once
#include "swrappers.hh"
#include <unistd.h>

int SConnectWithTimeout(int sockfd, const ComboAddress& remote, double timeout);

struct Socket
{
  Socket(int fd) : d_fd(fd){}
  Socket(int domain, int type, int protocol=0)
  {
    d_fd = SSocket(domain, type, protocol);
  }

  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;
  Socket(Socket&& rhs)
  {
    d_fd = rhs.d_fd;
    rhs.d_fd = -1;
  }

  operator int()
  {
    return d_fd;
  }
  
  void release()
  {
    d_fd=-1;
  }
  ~Socket()
  {
    if(d_fd >= 0)
      close(d_fd);
  }
  int d_fd;
};

/** Simple buffered reader for use on a non-blocking socket.
    Has only one method to access data, getChar() which either gets you a character or it doesn't.
    If the internal buffer is empty, SimpleBuffer will attempt to get up to bufsize bytes more in one go.
    Optionally, with setTimeout(seconds) a timeout can be set.

    WARNING: Only use ReadBuffer on a non-blocking socket! Otherwise it will
    block indefinitely to read 'bufsize' bytes, even if you only wanted one!
*/

class ReadBuffer
{
public:
  //! Instantiate on a non-blocking filedescriptor, with a given buffer size in bytes
  explicit ReadBuffer(int fd, int bufsize=2048) : d_fd(fd), d_buffer(bufsize)
  {}

  //! Set timeout in seconds
  void setTimeout(double timeout)
  {
    d_timeout = timeout;
  }

  //! Gets you a character in c, or false in case of EOF
  inline bool getChar(char* c)
  {
    if(d_pos == d_endpos && !getMoreData())
      return false;
    *c=d_buffer[d_pos++];
    return true;
  }

  inline bool haveData() const
  {
    return d_pos != d_endpos;
  }
  
private:
  bool getMoreData(); //!< returns false on EOF
  int d_fd;
  std::vector<char> d_buffer;
  unsigned int d_pos{0};
  unsigned int d_endpos{0};
  double d_timeout=-1;
};

/** Convenience class that requires a non-blocking socket as input and supports commonly used operations.
    SocketCommunicator will modify your socket to be non-blocking. This class
    will not close or otherwise modify your socket. 
    Note that since SocketCommunicator instantiates a buffered reader on your socket, you should not attempt concurrent reads on the socket as long as SocketCommunicator is active.
*/
class SocketCommunicator
{
public:
  //! Instantiate on top of this file descriptor, which will be set to non-blocking
  explicit SocketCommunicator(int fd) : d_rb(fd), d_fd(fd)
  {
    SetNonBlocking(fd);
  }
  //! Connect to an address, with the default timeout (which may be infinite)
  void connect(const ComboAddress& a)
  {
    SConnectWithTimeout(d_fd, a, d_timeout);
  }
  //! Get a while line of text. Returns false on EOF. Will return a partial last line. With timeout.
  bool getLine(std::string& line);

  //! Fully write out a message, even in the face of partial writes. With timeout.
  void writen(const std::string& message);

  //! Set the timeout (in seconds)
  void setTimeout(double timeout) { d_timeout = timeout; }
private:
  ReadBuffer d_rb;
  int d_fd;
  double d_timeout{-1};
};

// returns -1 in case if error, 0 if no data is available, 1 if there is
// negative time = infinity, timeout is in seconds
// should, but does not, decrement timeout
int waitForRWData(int fd, bool waitForRead, double* timeout=0, bool* error=0, bool* disconnected=0);

// should, but does not, decrement timeout. timeout in seconds
int waitForData(int fd, double* timeout=0);
