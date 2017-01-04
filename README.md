# simplesocket
simple socket helpers for C++ 2011 (or perhaps C++ 2014).

From C++ the full POSIX and OS socket functions are available.  These are
very powerful, offer well known semantics, but are somewhat of a pain to
use. 

Various libraries have attempted to offer "C++ native" socket environments,
but most of these offer different semantics that are less well known. Many
of them are also painful to use but in different ways.

This small set of files attempts to offer:

* A native datastructure that is understood by all socket calls, but easily
  manipulated and created form C++ (ComboAddress)
* Easier to use wrappers of the main BSD socket calls
* A few classes that build on the above to aid you achieve common tasks like
  reading a line from a socket to C++ friendly data structures

Each of these layers builds on the previous ones. If you want, you can only
use only ComboAddress, or only ComboAddress and the wrappers.

## History
ComboAddress was first described in a 2006
[blogpost](https://blog.netherlabs.nl/articles/2006/10/12/the-joys-of-mixing-c-and-c)
and has since become a part of PowerDNS. 

The simple wrappers have also long been used in various projects. The
top-most classes are new.

## Mission statements
### ComboAddress
ComboAddress is and will remain a minimal wrapper around the IPv4 and IPv6
sockaddrs. It will not address other address families.

ComboAddress will contain a few hashing primitives that facilitate storing
in hashed data structures. 

### Simple wrappers
These use the file descriptor of the socket as an object. In other words,
there is absolutely no state in our code, everything lives in the kernel.
The wrappers do not change semantics but only provide efficient variants
that can talk to C++ objects like ComboAddress, std::string (-wrappers).

### Simple classes
Operate on bare sockets. Do provide a minimal set of non-POSIX semantics,
like 'getline' on a TCP/IP socket, or 'writen' which deals with partial
writes.

Optionally knows about timeouts. If a class has state, this is noted very
clearly. This is done to make sure that sockets passed to instances can
continue to interoperate with all other socket calls.

## Status
Very early. API is likely to evolve. It is also not sure if this code will
depend on Boost and/or C++ 2014. C++ 2011 is a given.

How to report errors is also still an open question. This will likely end up
as a safe default which can globally be overwritten. 
