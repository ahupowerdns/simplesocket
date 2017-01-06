# The sockets API
I trawled the web and asked twitter for a brief introduction on how network sockets actually work in POSIX. I know of very long and thorough explanations (Stevens), medium length explanations (Beej), but no "tl;dr"-grade explanation. Here goes.

This assumes you know what IP addresses are and roughly the difference between UDP and TCP. 

However, even if you know the basics of the protocols, it turns out the API to the network is not obvious and full of traps. As an example, you can ask your operating system to transmit 1000KB of data over TCP/IP, and almost always, it will do that. However, a few percent of the time only only transmits 900KB. And if you did not check for that, your application is now subtly broken. Oh, and there is no flag that protects you against this.

If this is the kind of thing you need to know about, read on.

## Core concepts
The socket is the core concept. It is a filedescriptor, just like you'd get from opening a file. To some degree, a socket works like a file except when it doesn't.

Unlike files, sockets can be blocking or non-blocking. A non-blocking socket will never make you wait, and might not do everything you asked of it. A blocking socket will make you wait but might STILL not do everything you asked of it. In this document we start with the default of blocking sockets.

Sockets can be datagram (UDP) or stream (TCP). You will hear that UDP is unreliable, but TCP is not reliable either. If you write something to a TCP socket, there is no guarantee at all that anything happened to it, except that the kernel will do its best to get the data across. For UDP you don't even get that guarantee. This is in marked contrast to files where a write() and a sync() give you actual guarantees (if these work depends on your hardware).
## Error codes
With one notable exception (`getaddrinfo()`), all socket relevant system calls adhere to the C/POSIX/UNIX principle that a negative return code is an error. A 0 return code usually denotes EOF ('your connection is now gone'), a positive return tells you how much data got sent.

## Making a socket
You make a socket with the `socket(family, type, options)` call. Family can be AF_INET for IPv4, AF_INET6 for IPv6. Type is SOCK_DGRAM for UDP and SOCK_STREAM for TCP. Ignore the options for now.

## Anchoring a socket
Once you have a socket, it can be anchored at both ends: you can bind it to a local address, and connect it to a remote one. Both operations are sometimes optional (UDP) and sometimes mandatory (TCP).

### UDP
A UDP socket is ready for use once created with `socket()`. With `sendto(sock, content, contentlen, flags, destination, destinationlength)` you can start lobbing datagrams (which can end up as multiple packets). A packet gets sent or it doesn't. `sendto()` will let you know if it is sure it failed by returning negative. If it returns positive, well, it tried to send your packet is the best you know.

Receiving a packet can happen with `recvfrom(sock, content, room, origin*, originlen*)`. This will give you the contents of the received datagram and where it came from. NOTE! You will frequently get an error code instead of a packet. This happens for example if a packet did arrive, but on copying it to userspace, the kernel found the packet malformed somehow, but it will still wake up your program.

### TCP
TCP sockets need to be `connect()`-ed before you can send on them. In the words used above, you must anchor the remote end. Such a `connect()` can take significant time, minutes. More about this later. 

#### Sending (blocking)
Sending data over TCP is remarkably complicated. You can use `write()` or `send()` on a TCP socket (the latter has some more options). And frequently, if you ask it to send X bytes, it will actually queue up X bytes for transmission. However, also frequently, it will return it queued up only 0.1*X bytes. 

So the rule is: for any call to write(), you must check the return code for ALL THREE conditions: 
* < 0: there was an actual error
* 0: EOF, the connection is closed
* > 0: Some data was sent, but was it all data?

No matter how seasoned a programmer, you WILL forget about that last case from time to time. And this will lead to hard to debug issues that only crop up under heavy load and during the holiday season. No call to write() on a TCP socket can get away without dealing with partial writes!

#### Receiving (blocking)
Receiving data is the exact same story, but in reverse. All three error conditions must be checked, as it is entirely possible you did not get all the data you asked for. 

A common mistake is to forget that TCP/IP is actually a 'stream' protocol. So if you `write()` two lines of text on the sending side, this may show up on the receiving side all in one read(), or read() might first give you the first line and on the second invocation the second line.

In other words, data has no envelope. The information contained in the write()-boundaries is not reliably retained. 

#### Receiving a line of text
This is a ton of work to do reliably. If you do something like `read(s, buf, 1024)` in hopes of reading a line of at most 1024 characters, nothing will happen. That call will wait, for months if need be, until all 1024 bytes are in. If the line of text is only 80 characters long, you'll be waiting indefinitely.

The only way to do this reliably on a blocking TCP socket is.. to read one character at a time, and stop reading once you hit '\n'. This is astoundingly inefficient, but for now it is the only way.
