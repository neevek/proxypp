proxypp
=======

proxypp is a project that includes implementations of a SOCKS5 and an HTTP proxy server.

The HTTP proxy server supports specifying an upstream server, the upstream server can be a SOCKS5 proxy server or another HTTP proxy server, it also supports specifying domain name filter rules.

Dependencies
------------

The code depends on [uvcpp](https://github.com/neevek/uvcpp), which is a C++ wrapper of [libuv](https://github.com/libuv/libuv).

License
-------
MIT