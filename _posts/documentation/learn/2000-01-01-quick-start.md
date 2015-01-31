---
layout: post
title: Quick Start
categories: docs docs-learn
permalink: quick-start.html
---

This instruction assumes that PhantomJS is installed and its executable is placed somewhere in the PATH.

The code shown here is also available in [various examples]({{ site.url }}/examples/) included with PhantomJS. You are also recommended to explore the use of PhantomJS for [page automation]({{ site.url }}/page-automation.html), [network monitoring]({{ site.url }}/network-monitoring.html), [screen capture]({{ site.url }}/screen-capture.html), and [headless testing]({{ site.url }}/headless-testing.html).

## Hello, World!

Create a new text file that contains the following two lines:

```javascript
console.log('Hello, world!');
phantom.exit();
```

Save it as `hello.js` and then run it from the command line, not the REPL.

REPL is a simple, interactive computer programming environment. Read about REPL in the docs [Here]({{ site.url }}/repl.html). Again, REPL is the executable phantomjs.exe not the command line.

From the command prompt type:

```bash
phantomjs hello.js
```

The output is:

> Hello, world!

In the first line, `console.log` will print the passed string to the terminal. In the second line, `phantom.exit` terminates the execution.

It is **very important** to call `phantom.exit` at some point in the script, otherwise PhantomJS will not be terminated at all.

## Page Loading

A web page can be loaded, analyzed, and rendered by creating a web page object.

The following script demonstrates the simplest use of page object. It loads example.com and then saves it as an image, `example.png` in the same directory the script was run in.

```javascript
var page = require('webpage').create();
page.open('http://example.com', function(status) {
  console.log("Status: " + status);
  if(status === "success") {
    page.render('example.png');
  }
  phantom.exit();
});
```

Because of its rendering features, PhantomJS can be used to [capture web pages]({{ site.url }}/screen-capture.html), essentially taking a screenshot of the contents.

The following `loadspeed.js` script loads a specified URL (do not forget the http protocol) and measures the time it takes to load it.

```javascript
var page = require('webpage').create(),
  system = require('system'),
  t, address;

if (system.args.length === 1) {
  console.log('Usage: loadspeed.js <some URL>');
  phantom.exit();
}

t = Date.now();
address = system.args[1];
page.open(address, function(status) {
  if (status !== 'success') {
    console.log('FAIL to load the address');
  } else {
    t = Date.now() - t;
    console.log('Loading ' + system.args[1]);
    console.log('Loading time ' + t + ' msec');
  }
  phantom.exit();
});
```

Run the script with the command:

```bash
phantomjs loadspeed.js http://www.google.com
```

It outputs something like:

> Loading http://www.google.com
> Loading time 719 msec

## Code Evaluation

To evaluate JavaScript code in the context of the web page, use `evaluate()` function. The execution is "sandboxed", there is no way for the code to access any JavaScript objects and variables outside its own page context. An object can be returned from `evaluate()`, however it is limited to simple objects and can't contain functions or closures.

Here is an example to show the title of a web page:

```javascript
var page = require('webpage').create();
page.open(url, function(status) {
  var title = page.evaluate(function() {
    return document.title;
  });
  console.log('Page title is ' + title);
  phantom.exit();
});
```

Any console message from a web page, including from the code inside `evaluate()`, will not be displayed by default. To override this behavior, use the `onConsoleMessage` callback. The previous example can be rewritten to:

```javascript
var page = require('webpage').create();
page.onConsoleMessage = function(msg) {
  console.log('Page title is ' + msg);
};
page.open(url, function(status) {
  page.evaluate(function() {
    console.log(document.title);
  });
  phantom.exit();
});
```

Since the script is executed as if it is running on a web browser, standard DOM scripting and CSS selectors work just fine. It makes PhantomJS suitable to carry out various [page automation tasks]({{ site.url }}/page-automation.html).

## Network Requests and Responses

When a page requests a resource from a remote server, both the request and the response can be tracked via `onResourceRequested` and `onResourceReceived` callback. This is demonstrated in the example [netlog.js](https://github.com/ariya/phantomjs/blob/master/examples/netlog.js):

```javascript
var page = require('webpage').create();
page.onResourceRequested = function(request) {
  console.log('Request ' + JSON.stringify(request, undefined, 4));
};
page.onResourceReceived = function(response) {
  console.log('Receive ' + JSON.stringify(response, undefined, 4));
};
page.open(url);
```

For more info on how to utilize this features for HAR export as well as YSlow-based performance analysis, see the page on [network monitoring]({{ site.url }}/network-monitoring.html).
