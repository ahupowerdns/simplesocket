#include <iostream>
#include "comboaddress.hh"
#include "swrappers.hh"
#include "sclasses.hh"
#include <fmt/format.h>
#include <fmt/printf.h>
using std::cout;
using std::endl;

void test0()
{
  int s = SSocket(AF_INET, SOCK_STREAM);
  ComboAddress webserver("52.48.64.3", 80);
  SConnect(s, webserver);
  SWrite(s, "GET / HTTP/1.1\r\nHost: ds9a.nl\r\nConnection: Close\r\n\r\n"); // will throw on partial write
  std::string response = SRead(s);
  cout<< response << endl;
  close(s);
}

void test1()
{
  int s = SSocket(AF_INET, SOCK_STREAM, 0);
    
  SBind(s, "0.0.0.0:1024"_ipv4);
  ComboAddress remote("52.48.64.3", 80);
  SConnect(s, "52.48.64.3:80"_ipv4);
  SetNonBlocking(s);
  SocketCommunicator sc(s);
  sc.writen("GET / HTTP/1.1\r\nHost: ds9a.nl\r\nConnection: Close\r\n\r\n");
  std::string line;
  while(sc.getLine(line)) {
    cout<<"Got: "<<line;
  }
  close(s);

}

void test2()
{
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
}

void test3()
{
  std::vector<Socket> listeners;
  for(int n=10000; n < 10003; ++n) {
    Socket sock(AF_INET6, SOCK_STREAM);
    SSetsockopt(sock, SOL_SOCKET, SO_REUSEADDR, true);
    SBind(sock, ComboAddress("::1", n));
    SListen(sock, 10);
    listeners.push_back(std::move(sock));
  }
  
  for(int t=0; t < 5; ++t) {
    auto av = SPoll({listeners[0], listeners[1], listeners[2]}, {}, 1.0);
    for(auto& a : av) {
      ComboAddress remote;
      remote.sin.sin_family = AF_INET6;
      Socket rs=SAccept(a.first, remote);
      SWriten(rs, "Hello!\r\n");
    }
  }
}

void test4()
{
  auto addresses = resolveName("www.timeapi.org"); // gets IPv4 and IPv6
  if(addresses.empty()) {
    std::cout <<"Could not resove www.timeapi.org name\n";
    return;
  }
  for(auto& a : addresses) {
    try {
      a.setPort(80);
      Socket rs(a.sin.sin_family, SOCK_STREAM);

      SocketCommunicator sc(rs);
      sc.setTimeout(1.5);
      sc.connect(a);
      sc.writen("GET /utc/now HTTP/1.1\r\nHost: www.timeapi.org\r\nUser-Agent: simplesocket\r\nReferer: http://www.timeapi.org/\r\nConnection: Close\r\n\r\n");
      std::string line;
      bool inheader=true;
      while(sc.getLine(line)) {
        auto pos = line.find_last_of(" \r\n");
        if(pos != std::string::npos)
          line.resize(pos);
          
        if(inheader && line.empty()) {
          inheader=false;
          continue;
        }
        if(!inheader)
          std::cout << line << std::endl;
      }
      return;
    }
    catch(std::exception& e) {
      std::cerr <<"Failed to retrieve time from "<<a.toStringWithPort()<<"\n";
    }
  }
  std::cerr <<"Failed to retrieve time\n";
}


int main()
{
  test3();
  test0();
  /*
  test4();
  test3();
  test1();
  test2();
  */
}
