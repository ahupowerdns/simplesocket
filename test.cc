#include <iostream>
#include "comboaddress.hh"
#include "swrappers.hh"
#include "sclasses.hh"
#include <boost/algorithm/string.hpp>
using std::cout;
using std::endl;

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
    
    RAIISocket rs(a.sin.sin_family, SOCK_STREAM);
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
      RAIISocket rs(a.sin.sin_family, SOCK_STREAM);

      SocketCommunicator sc(rs);
      sc.setTimeout(1.5);
      sc.connect(a);
      sc.writen("GET /utc/now HTTP/1.1\r\nHost: www.timeapi.org\r\nUser-Agent: simplesocket\r\nReferer: http://www.timeapi.org/\r\nConnection: Close\r\n\r\n");
      std::string line;
      bool inheader=true;
      while(sc.getLine(line)) {
        boost::trim(line);
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
  test4();
  test3();
  test1();
  test2();
  
}
