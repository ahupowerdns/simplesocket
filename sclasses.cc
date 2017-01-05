#include "sclasses.hh"
#include <assert.h>
#include <sys/poll.h>

// returns -1 in case if error, 0 if no data is available, 1 if there is
// negative time = infinity
int waitForRWData(int fd, bool waitForRead, int seconds=-1, int useconds=-1)
{
  int ret;

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fd;

  if(waitForRead)
    pfd.events=POLLIN;
  else
    pfd.events=POLLOUT;

  ret = poll(&pfd, 1, seconds * 1000 + useconds/1000);
  if ( ret == -1 ) {
    throw std::runtime_error("Waiting for data: "+std::string(strerror(errno)));
  }
  return ret;
}

int waitForData(int fd, int seconds=-1, int useconds=-1)
{
  return waitForRWData(fd, true, seconds, useconds);
}



bool ReadBuffer::getMoreData()
{
  assert(d_pos == d_endpos);
  for(;;) {
    int res = read(d_fd, &d_buffer[0], d_buffer.size());
    if(res < 0 && errno == EAGAIN) {
      waitForData(d_fd);
      continue;
    }
    if(res < 0)
      throw std::runtime_error("Getting more data: " + std::string(strerror(errno)));
    if(!res)
      return false;
    d_endpos = res;
    d_pos=0;
    break;
  }
  return true;
}

bool SocketCommunicator::getLine(std::string& line)
{
  line.clear();
  char c;
  while(d_rb.getChar(&c)) {
    line.append(1, c);
    if(c=='\n')
      return true;
  }
  return !line.empty();
}

void SocketCommunicator::writen(boost::string_ref content)
{
  unsigned int pos=0;
  
  int res;
  while(pos < content.size()) {
    res=write(d_fd, &content[pos], content.size()-pos);
    if(res < 0) {
      if(errno == EAGAIN) {
        waitForRWData(d_fd, false);
        continue;
      }
      throw std::runtime_error("Writing to socket: "+std::string(strerror(errno)));
    }
    if(res==0)
      throw std::runtime_error("EOF on write");
    pos += res;
  }
    
}
