#pragma once
#include <boost/utility/string_ref.hpp>

#include "comboaddress.hh"
int SSocket(int family, int type, int flags=0);
int SConnect(int sockfd, const ComboAddress& remote);
int SBind(int sockfd, const ComboAddress& local);
int SAccept(int sockfd, ComboAddress& remote);
int SListen(int sockfd, int limit);
int SSetsockopt(int sockfd, int level, int opname, int value);
void SetNonBlocking(int sockfd, bool to=true);

std::vector<ComboAddress> resolveName(boost::string_ref name, bool ipv4=true, bool ipv6=true);

