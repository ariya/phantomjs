---
layout: post
title: Examples
categories: docs docs-explore
permalink: examples/index.html
---

PhantomJS comes with a lot of [included examples](https://github.com/ariya/phantomjs/tree/master/examples).

## Basic examples

* [arguments.js](https://github.com/ariya/phantomjs/blob/master/examples/arguments.js) shows the arguments passed to the script
* [countdown.js](https://github.com/ariya/phantomjs/blob/master/examples/countdown.js) prints a 10 second countdown
* [echoToFile.js](https://github.com/ariya/phantomjs/blob/master/examples/echoToFile.js) writes the command line arguments to a file
* [fibo.js](https://github.com/ariya/phantomjs/blob/master/examples/fibo.js) lists the first few numbers in the Fibonacci sequence
* [hello.js](https://github.com/ariya/phantomjs/blob/master/examples/hello.js) displays the famous message
* [module.js](https://github.com/ariya/phantomjs/blob/master/examples/module.js) and `universe.js` demonstrate the use of module system
* [outputEncoding.js](https://github.com/ariya/phantomjs/blob/master/examples/outputEncoding.js) displays a string in various encodings
* [printenv.js](https://github.com/ariya/phantomjs/blob/master/examples/printenv.js) displays the system's environment variables
* [scandir.js](https://github.com/ariya/phantomjs/blob/master/examples/scandir.js) lists all files in a directory and its subdirectories
* [sleepsort.js](https://github.com/ariya/phantomjs/blob/master/examples/sleepsort.js) sorts integers and delays display depending on their values
* [version.js](https://github.com/ariya/phantomjs/blob/master/examples/version.js) prints out PhantomJS version number
* [page_events.js](https://github.com/ariya/phantomjs/blob/master/examples/page_events.js) prints out page events firing: useful to better grasp `page.on*` callbacks

## Rendering/rasterization

* [colorwheel.js](https://github.com/ariya/phantomjs/blob/master/examples/colorwheel.js) creates a color wheel using HTML5 canvas
* [rasterize.js](https://github.com/ariya/phantomjs/blob/master/examples/rasterize.js) rasterizes a web page to image or PDF
* [render_multi_url.js](https://github.com/ariya/phantomjs/blob/master/examples/render_multi_url.js) renders multiple web pages to images

## Page automation

* [injectme.js](https://github.com/ariya/phantomjs/blob/master/examples/injectme.js) injects itself into a web page context
* [phantomwebintro.js](https://github.com/ariya/phantomjs/blob/master/examples/phantomwebintro.js) uses jQuery to read #intro element text from phantomjs.org
* [unrandomize.js](https://github.com/ariya/phantomjs/blob/master/examples/unrandomize.js) modifies a global object at page initialization
* [waitfor.js](https://github.com/ariya/phantomjs/blob/master/examples/waitfor.js) waits until a test condition is true or a timeout occurs

## Network

* [detectsniff.js](https://github.com/ariya/phantomjs/blob/master/examples/detectsniff.js) detects if a web page sniffs the user agent
* [loadspeed.js](https://github.com/ariya/phantomjs/blob/master/examples/loadspeed.js) computes the loading speed of a web site
* [netlog.js](https://github.com/ariya/phantomjs/blob/master/examples/netlog.js) dumps all network requests and responses
* [netsniff.js](https://github.com/ariya/phantomjs/blob/master/examples/netsniff.js) captures network traffic in HAR format
* [post.js](https://github.com/ariya/phantomjs/blob/master/examples/post.js) sends an HTTP POST request to a test server
* [postserver.js](https://github.com/ariya/phantomjs/blob/master/examples/postserver.js) starts a web server and sends an HTTP POST request to it
* [server.js](https://github.com/ariya/phantomjs/blob/master/examples/server.js) starts a web server and sends an HTTP GET request to it
* [serverkeepalive.js](https://github.com/ariya/phantomjs/blob/master/examples/serverkeepalive.js) starts a web server which answers in plain text
* [simpleserver.js](https://github.com/ariya/phantomjs/blob/master/examples/simpleserver.js) starts a web server which answers in HTML

## Testing

* [run-jasmine.js](https://github.com/ariya/phantomjs/blob/master/examples/run-jasmine.js) runs Jasmine based tests
* [run-qunit.js](https://github.com/ariya/phantomjs/blob/master/examples/run-qunit.js) runs QUnit based tests

## Browser

* [features.js](https://github.com/ariya/phantomjs/blob/master/examples/features.js) detects browser features using `modernizr.js`
* [useragent.js](https://github.com/ariya/phantomjs/blob/master/examples/useragent.js) changes the browser's user agent property

(More to be written)
