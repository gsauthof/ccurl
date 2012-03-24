This repository contains a simple C++ wrapper for the [cURL][1]
API (cURL is a powerful C library for creating HTTP clients -
e.g. for GETing and POSTing stuff).

## Usage ##

Retrieve a URL into a string:

    curl::global g;
    std::string s;
    curl::calback::string cb(s);
    curl::handle h(g, cb);
    h.get("file://curl.hh");
    std::cout << s;

Retrieve a URL into a vector:

    curl::global g;
    std::vector v;
    curl::calback::vector cb(v);
    curl::handle h(g, cb);
    h.get("http:://lwn.net");

There is also a predefined callback class for `std::ostream`s -
you get the idea.

## Features ##

Usage of namespaces, exceptions and setting some sane defaults
for cURL (see the code).

## Contact ##

Georg Sauthoff <mail@georg.so>

## License ##

GPVv3+


[1]: http://curl.haxx.se/
