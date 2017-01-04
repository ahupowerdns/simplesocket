#include <iostream>
#include "comboaddress.hh"
#include "swrappers.hh"
#include "sclasses.hh"

using std::cout;
using std::endl;

void test1()
{
  int s = socket(AF_INET, SOCK_STREAM, 0);
    
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
    cout<<"Connecting to: " << a.toStringWithPort() << endl;
    
    int s = SSocket(a.sin.sin_family, SOCK_STREAM, 0);
    SConnect(s, a);
    SetNonBlocking(s);
    SocketCommunicator sc(s);
    
    sc.writen("GET / HTTP/1.1\r\nHost: ds9a.nl\r\nConnection: Close\r\n\r\n");

    std::string line;
    while(sc.getLine(line)) {
      cout<<"Got: "<<line;
    }
    close(s);
  }

}


int main()
{
  test1();
  test2();
  
}
