#include "sclasses.hh"
#include <assert.h>
#include <sys/poll.h>
#include <fmt/format.h>
#include <fmt/printf.h>

int waitForRWData(int fd, bool waitForRead, double* timeout, bool* error, bool* disconnected)
{
  int ret;

  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fd;

  if(waitForRead)
    pfd.events=POLLIN;
  else
    pfd.events=POLLOUT;

  ret = poll(&pfd, 1, timeout ? (*timeout * 1000) : -1);
  if ( ret == -1 ) {
    throw std::runtime_error("Waiting for data: "+std::string(strerror(errno)));
  }
  if(ret > 0) {
    if (error && (pfd.revents & POLLERR)) {
      *error = true;
    }
    if (disconnected && (pfd.revents & POLLHUP)) {
      *disconnected = true;
    }

  }
  return ret;
}

int waitForData(int fd, double* timeout)
{
  return waitForRWData(fd, true, timeout);
}

int SConnectWithTimeout(int sockfd, const ComboAddress& remote, double timeout=-1)
{
  int ret = connect(sockfd, (struct sockaddr*)&remote, remote.getSocklen());
  if(ret < 0) {
    int savederrno = errno;
    if (savederrno == EINPROGRESS) {
      /* we wait until the connection has been established */
      bool error = false;
      bool disconnected = false;
      int res = waitForRWData(sockfd, false, &timeout, &error, &disconnected);
      if (res == 1) {
        if (error) {
          savederrno = 0;
          socklen_t errlen = sizeof(savederrno);
          if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *)&savederrno, &errlen) == 0) {
            throw std::runtime_error(fmt::sprintf("connecting to %s failed: %s", remote.toStringWithPort(), strerror(savederrno)));
          }
          else {
            throw std::runtime_error(fmt::sprintf("connecting to %s failed", remote.toStringWithPort()));
          }
        }
        if (disconnected) {
          throw std::runtime_error(fmt::sprintf("%s closed the connection", remote.toStringWithPort()));;
        }
        return 0;
      }
      else if (res == 0) {
        throw std::runtime_error(fmt::sprintf("timeout while connecting to %s", remote.toStringWithPort()));
      } else if (res < 0) {
        savederrno = errno;
        throw std::runtime_error(fmt::sprintf("waiting to connect to %s: %s", remote.toStringWithPort(), strerror(savederrno)));
      }
    }
    else {
      throw std::runtime_error(fmt::sprintf("connecting to %s: %s", remote.toStringWithPort(), strerror(savederrno)));
    }
  }

  return ret;
}


bool ReadBuffer::getMoreData()
{
  assert(d_pos == d_endpos);
  double timeout = d_timeout;
  for(;;) {
    int res = read(d_fd, &d_buffer[0], d_buffer.size());
    if(res < 0 && errno == EAGAIN) {
      waitForData(d_fd, &timeout);
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

void SocketCommunicator::writen(const std::string& content)
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
