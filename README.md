# simplesocket
simple socket helpers for C++ 2014

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

