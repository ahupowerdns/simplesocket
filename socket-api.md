# The sockets API
I trawled the web and asked Twitter for a brief introduction on how network sockets actually work in POSIX. I know of very long and thorough explanations ([Stevens](https://www.amazon.com/W.-Richard-Stevens/e/B000AP9GV4)), medium length explanations ([Beej](http://beej.us/guide/bgnet/)), but no "tl;dr"-grade page. Here goes.

The goal is to provide a quick intro to sufficient parts of the socket API so that one can write competent and robust TCP or UDP client or server programs in any language that offers access to this API.

NOTE: This document is a companion to an [easy to use set of C++ helpers](https://github.com/ahupowerdns/simplesocket) that give you the exact socket API, but with less typing. 

The following assumes you know what IP addresses & ports are, and roughly the difference between UDP and TCP. 

However, even if you know the basics of the protocols, it turns out the API to the network is not obvious and full of traps. As an example, you can ask your operating system to transmit 100KB of data over TCP/IP, and almost always, it will do that. However, a few percent of the time it only transmits 90KB. And if you did not check for that, your application is now subtly broken. 

If this is the kind of thing you need to know about, read on.

## Core concepts
The socket is the core concept. It is a file descriptor, just like you'd get from opening a file. To some degree, a socket works like a file except when it doesn't.

Unlike files, sockets can be blocking or non-blocking. A non-blocking socket will never make you wait, and might not do everything you asked of it. A blocking socket will make you wait but might STILL not do everything you asked of it. In this document we start with the default of blocking sockets.

We'll only be discussing datagram (UDP) or stream (TCP) sockets here. You will have read that UDP is unreliable, but it turns out TCP is not reliable either. If you write something to a TCP socket, there is no guarantee at all that anything happened to it, except that the kernel will do its best to get the data across. For UDP you don't even get that promise. This is in marked contrast to files where a `write()` and a `sync()` give you actual guarantees (if these work depends on your hardware).

## Error codes
With the exception of the address resolution functions, all socket relevant system calls adhere to the C/POSIX/UNIX principle that a negative return code is an error. A 0 return code usually denotes EOF ('your connection is now gone'), a positive return tells you how much data got sent.

There is not a single socket call where you do not need to check the return code. All of them, including `close()` frequently return errors you need to know about.

## Making a socket & disposing of it
You make a socket with the `socket(family, type, options)` call. Family can be `AF_INET` for IPv4, `AF_INET6` for IPv6. Type is `SOCK_DGRAM` for UDP and `SOCK_STREAM` for TCP. Ignore the options for now. 

Once done with a socket, you can call `close()` on it. There is usually no need to call `shutdown()` on a socket, nor might this do anything useful.

## Addresses
Network addresses, including port number & other parameters (for IPv6) live in `struct sockaddr_in` and `struct sockaddr_in6`. Since C does not do inheritance or polymorphism, this is emulated. Both structs start the same, and all calls accept a pointer to a `struct sockaddr` and a length field. From this, the operating system learns if it is looking at an IPv4 or an IPv6 (or another) address.

Filling out these sockaddr structs is far more complicated than you'd expect, but we'll get to that. 

## Anchoring a socket
Once you have a socket, it can be anchored at both ends: you can bind it to a local address, and connect it to a remote one. Anchoring is sometimes optional (UDP) and sometimes mandatory (TCP).

### UDP
A UDP socket is ready for use once created with `socket()`. With `sendto(sock, content, contentlen, flags, sockaddr*, sockaddrlength)` you can start lobbing datagrams (which can end up as multiple packets due to IP fragmentation). A packet either gets sent or it does not. `sendto()` will let you know if it is sure it failed by returning negative. If it returns positive, well, it tried to send your packet is the best you know.

Receiving a packet can happen with `recvfrom(sock, content, room, sockaddr*, sockaddrlen*)`. This will give you the contents of the received datagram and where it came from. NOTE: You will frequently get an error code instead of a packet. This happens for example if a packet did arrive, but on copying it to userspace, the kernel found the packet malformed somehow, but it will still wake up your program.

UDP sockets can also be anchored on one end or both ends. With `bind(sock, sockaddr*, sockadrlen)` the local end can be bound to an IP address and/or port number. With `connect()` and the same parameters, the other end is fixed. No actual packet gets sent to set up this 'connection', but there are benefits to a connected() UDP socket anyhow, like far better error reporting.

### TCP client (blocking)
TCP sockets need to be `connect()`-ed before you can send on them. In the words used above, this means you must anchor the remote end. Such a `connect()` can take significant time, minutes. More about this later. You can anchor the local end as well with `bind()` if you want to pick the IP address or source port. 

### TCP server (blocking)
To receive incoming connections, a TCP socket must first be anchored locally with `bind(sock, sockaddr*, sockaddrlen)`. Then we must inform the kernel we want to `listen(s, amount)` to incoming connections. Finally, we can then call `accept(sock, sockaddr*, sockaddrlen*)` to get new sockets that represent incoming connections.

NOTE: See the remark about `SO_REUSEADDR` below. 

NOTE: like `recvfrom()` and other socket calls, `accept()` will frequently surprise you with unexpected errors. This happens for example when a valid connection arrived, but it got disconnected before `accept()` returns. Typically, `accept()` is also the call that tells you you ran out of file descriptors, and there is no recovery from that. Naive code will retry `accept()` on `EMFILE` and thus make a bad situation worse. 

#### Sending (blocking)
Sending data over TCP is remarkably complicated. You can use `write()` or `send()` on a TCP socket (the latter has some more options). And frequently, if you ask it to send X bytes, it will actually queue up X bytes for transmission. However, also frequently, it will return it queued up only 0.1*X bytes. 

So the rule is: for any call to `write()`, you must check the return code for ALL THREE conditions:

* < 0: there was an actual error
* 0: EOF, the connection is closed
* \> 0: Some data was sent, but was it all of the data?

No matter how seasoned a programmer, you WILL forget about that last case from time to time. And this will lead to hard to debug issues that only crop up under heavy load and during the holiday season. No call to `write()` on a TCP socket can get away without dealing with partial writes!

Once `write()` or `send()` returns you have zero guarantees about what actually happened. The data might be sent, it might not be sent, the remote connection might have gone away. Reliable, eh?

#### Receiving (blocking)
Receiving data is the exact same story, but in reverse. All three error conditions must again be checked, as it is entirely possible you did not get all the data you asked for. 

A common mistake is to forget that TCP/IP is actually a 'stream' protocol. So if you `write()` two lines of text on the sending side, this may show up on the receiving side all in one `read()`, or `read()` might first give you the first line and on the second invocation the second line, or even give you 0.5 and then 1.5 line.

In other words, the data has no envelope. The information contained in the `write()`-boundaries is not reliably retained, if at all.

#### Receiving a line of text
This is a ton of work to do reliably. If you do something like `read(s, buf, 1024)` in hopes of reading a line of at most 1024 characters, nothing will happen. That call will wait, for months if need be, until all 1024 bytes are in. If the line of text is only 80 characters long, you'll be waiting indefinitely.

The only way to do this reliably on a blocking TCP socket is.. to read one character at a time, and stop reading once you hit '\\n'. This is astoundingly inefficient, but for now it is the only way.

## Non-blocking sockets
Blocking sockets will make you wait, but might not do everything you ask of them anyhow. Non-blocking sockets only promise to not make you wait. In a sense, this makes them easier to use as they will disappoint you far quicker and break your bad code in obvious ways.

To make a socket non-blocking in a portable way, do:
```
flags=fcntl(sock, F_GETFL, 0);  // DO CHECK ERRORS!
flags |= O_NONBLOCK;
flags=fcntl(sock, F_SETFL, flags);  // SERIOUSLY
```
### TCP (non-blocking)
If you try to read from a non-blocking TCP socket, you instantly get data or an error code. No matter how much data you asked for, the call could return just 1 byte. If there is no data for you, it will not respond with 0. 0 always means EOF. If there is no data, you'll receive a negative return, and `errno` will be set to `EAGAIN`.

The exact same thing happens on a write. Either at least 1 byte was submitted for transmission, and the amount submitted gets returned, or you get 0 denoting EOF, or you get an error condition and `EAGAIN`. 

Of course you could still get all the other error conditions as well.

So, if you try to read a full line of text (as described above), you can now safely ask for 1024 bytes, and if there is any data for you, you'll get it. The call will not block. You might get two lines of text though, or only a part of the line. But there is no more need to read one byte at a time. 

### UDP (non-blocking)
This works much like blocking UDP, alhough you might now also get `EAGAIN` when trying to receive data. At least there is no waiting.

## Waiting
So, if we got EAGAIN, now what? Almost all sample code will tell you to use `select()` to wait on a socket, but no one should ever be using `select()` ever again, since this call breaks violently if you use more than x file descriptors. You'll find out x at 3AM one day. Use `poll()` instead, or one of the more modern variants (epoll, kqueue).

It is important to note that POSIX takes a broad view of what denotes 'readable' or 'writable' on a socket. For example, EOF is both. So `poll()` might return and tell you a socket is readable, but when you try, it gives you EOF. 

In addition, like many other functions, `poll()` frequently changes its mind. If it says a socket is readable or writable, this may no longer be the case by the time you try. For this reason, do not EVER use `poll()` on a blocking socket. You might block.

One can also do non-blocking `connect()` but the details are arcane. The error code of a failed connect needs to be retrieved with additional work. Sample code is in [SimpleSocket](https://github.com/ahupowerdns/simplesocket/blob/ed53be41f5bf5722cda428ae32205b4fe9576d4e/sclasses.cc#L44).

## Filling out struct sockaddr_in and struct sockaddr_in6
To actually connect or send something, we'll need to fill out these sockaddr structs. This is surprisingly tricky.
First, you must make sure the IPv6 struct is zeroed out as it is full of fields that need to be set to safe defaults.

Secondly, the function recommended to fill out these structs (`getaddrinfo()`) has suffered from mission creep. It is safer to use the older functions inet_pton and inet_aton if you can get away with them. Sample code:

```
struct sockaddr_in ret;
inet_aton("127.0.0.1", &ret.sin_addr); // DO CHECK FOR ERRORS
ret.sin.sin_port = htons(80);          // htons has historical reasons
```

And for IPv6:
```
struct sockaddr_in6 ret;
memset(&ret, 0, sizeof(ret));
inet_pton(AF_INET6, "::1", (void*)&ret.sin6_addr);
ret.sin6.sin6_port = htons(80);  
```

`inet_pton` however does not deal with scoped IPv6 addresses. The full horror of converting IPv6 addresses is explained [here](https://blog.powerdns.com/2014/05/21/a-surprising-discovery-on-converting-ipv6-addresses-we-no-longer-prefer-getaddrinfo/)

## A note on reliability and "flushing" or "syncing" data
No matter how you do it, the socket API will never guarantee you that your data arrived. In fact it will not even guarantee you that it got _sent_. There are various system calls that make it *more likely* that your data hit the wire. But no promises. If you ever want to be sure your data made it to the other end, the only way to find that out is if the other end actually confirms this to you.

Some useful and simple tricks to make this happen can be found [here](https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable)

## Some vital historical code
You will need to run the following in all network code:
```
signal(SIGPIPE, SIG_IGN);
```
Otherwise your code will silently die occasionally for reasons that arre hard to explain. Just say no. 

Additionally, for any TCP socket you call `listen()` on, you must first set the socket option SO_REUSEADDR to 1. This too has valid historical reasons which are hard to explain, but just do it. 

```
int one=1;
setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,(char*)&one,sizeof one);
```

## If you want to know more
This page, by design, only gives an introduction to the basics. Thousand page books have been written about networking, however.

[Beej](http://beej.us/guide/bgnet/) goes into intermediate depth. The glorious books by [Stevens](https://www.amazon.com/W.-Richard-Stevens/e/B000AP9GV4) are fantastic. The Open Group manpages for the various calls are exhaustive, as is [The Linux Programming Interface](http://man7.org/tlpi/).


