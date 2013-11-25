---
layout: post
title: Inter Process Communication
categories: docs docs-learn
permalink: inter-process-communication.html
---

There are multiple ways to handle inter-process communication between PhantomJS and other processes.

## File I/O

* files
* sockets (is there a general-purpose way to open a socket in PhantomJS? If so, we need a link here. Maybe there is not as of 1.9.0)
* named pipes

## HTTP

* outgoing HTTP requests (to other processes/services)
  * GET/POST data to a server endpoint
  * GET/POST data to a server endpoint, parse resulting JSON/XML/HTML/etc.
  * Create a XMLHttpRequest object and use it as you would in a normal web page
  * Cross-domain requests are restricted by default (PhantomJS code is considered to be in file:// scope). Use Access-Control-Allow-Origin on your server to enable access.

* incoming HTTP requests (server). Use the [[API Reference WebServer]] webserver module
* route PhantomJS traffic through an HTTPS proxy like mitmproxy/libmproxy

## Websockets inside a WebPage context

* hixie-76 websockets only
* PhantomJS 2.x will eventually get a WebKit upgrade that has RFC 6455 websockets.

## stdin, stdout, and stderr

See the [stdin-stdout-stderr.js](https://github.com/ariya/phantomjs/blob/master/examples/stdin-stdout-stderr.js) example script for details.
