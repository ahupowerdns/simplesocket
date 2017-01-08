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

Documentation (by Doxygen) is [here](https://ds9a.nl/simplesocket).

## Sample code
Some sample code using all three elements:
```
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
```
This shows the use of `resolveName()` which when called like this retrieves
IPv4 and IPv6 addresses. 

In the for loop, we set the destination port to 80.  The `Socket` class
creates a socket that is closed when `rs` goes out of scope. `Socket`
has no state, it is an int. It can not be copied, but it can be moved into
a container.

The `SocketCommunicator` class provides an interface that provides for
sending of whole messages (even in the face of partial writes) and
retrieving whole lines (even if they appear in multiple reads).
`SocketCommunicator` als supports timeouts.

## Using the code
Simply compile & link the .cc files into your project.

## History
ComboAddress was first described in a 2006
[blogpost](https://blog.netherlabs.nl/articles/2006/10/12/the-joys-of-mixing-c-and-c)
and has since become a part of PowerDNS. 

The simple wrappers have also long been used in various projects. The
top-most classes are new.

## Mission statements
To repeat, the goal of this library is in no way to sugarcoat the BSD socket
API. The underlying semantics are exposed to you 100%, including the
'tristate' return codes for operations. A 0 means EOF, a negative value is
an error. A positive value continues to mean what it meant.

We do translate the error conditions to exceptions. But an EOF is not an
error.

The underlying socket API is not straightforward, but it is well documented.
In this library, it is exactly what you get. Except with less typing.

### ComboAddress
ComboAddress is and will remain a minimal wrapper around the IPv4 and IPv6
sockaddrs. It will not address other address families.

ComboAddress will contain a few hashing primitives that facilitate storing
in hashed data structures. 

Sample code, showing how to use ComboAddress with the native socket API:
```
ComboAddress aroot("198.41.0.4", 53);
sendto(s, "hello", 5, 0, (struct sockaddr*)&aroot, aroot.getSocklen());
...
ComboAddress client("::");
socklen_t slen = client.getSocklen();

csock = accept(s, (struct sockaddr*)&client, &slen);
```


### Simple wrappers
These use the file descriptor of the socket as an object. In other words,
there is absolutely no state in our code, everything lives in the kernel.
The wrappers do not change semantics but only provide efficient variants
that can talk to C++ objects like ComboAddress and std::string (-wrappers).

Sample code:
```
s = SSocket(AF_INET, SOCK_DGRAM);
ComboAddress aroot("198.41.0.4", 53);
SConnect(s, aroot);
SSend(s, "hello");
```

Regular return codes get returned, negative return codes get turned into
exceptions. EOF is not an error.

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
