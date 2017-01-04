#include "comboaddress.hh"
#include "swrappers.hh"

int main()
{
  int s = socket(AF_INET, SOCK_STREAM, 0);

  SBind(s, "0.0.0.0:1024"_ipv4);
  ComboAddress remote("52.48.64.3", 80);
  SConnect(s, "52.48.64.3:80"_ipv4);

}
