---
layout: post
title: Inter Process Communication
categories: docs docs-learn
permalink: inter-process-communication.html
---

There are multiple ways to handle inter-process communication between PhantomJS and other processes.

## File I/O

* [files](http://phantomjs.org/api/fs/)

## HTTP

* outgoing HTTP requests (to other processes/services)
  * GET/POST data to a server endpoint
  * GET/POST data to a server endpoint, parse resulting JSON/XML/HTML/etc.
  * Create a XMLHttpRequest object and use it as you would in a normal web page
  * Cross-domain requests are restricted by default (PhantomJS code is considered to be in file:// scope). Use [Access-Control-Allow-Origin](http://www.w3.org/TR/cors/#access-control-allow-origin-response-header) on your server to enable access.

* incoming HTTP requests (server). Use the [WebServer module](http://phantomjs.org/api/webserver/)
* route PhantomJS traffic through an HTTPS proxy like [mitmproxy/libmprox](http://mitmproxy.org/) or [fiddler](http://fiddler2.com/)

## Websockets inside a WebPage context

* [hixie-76](http://tools.ietf.org/html/draft-hixie-thewebsocketprotocol-76) websockets only
* PhantomJS 2.x will eventually get a WebKit upgrade that has [RFC 6455](http://tools.ietf.org/html/rfc6455) websockets.

## stdin, stdout, and stderr

See the [stdin-stdout-stderr.js](https://github.com/ariya/phantomjs/blob/master/examples/stdin-stdout-stderr.js) example script for details.
