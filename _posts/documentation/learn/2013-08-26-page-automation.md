---
layout: post
title: Page Automation
categories: docs docs-learn
permalink: page-automation.html
---

Because PhantomJS can load and manipulate a web page, it is perfect to carry out various page automations.

## DOM Manipulation

Since the script is executed as if it is running on a web browser, standard [DOM scripting](http://en.wikipedia.org/wiki/DOM_scripting) and [CSS selectors](http://www.w3.org/TR/css3-selectors/) work just fine.

The following `useragent.js` example demonstrates reading the `textContent` property of the element whose *id* is `myagent`:

```javascript
var page = require('webpage').create();
console.log('The default user agent is ' + page.settings.userAgent);
page.settings.userAgent = 'SpecialAgent';
page.open('http://www.httpuseragent.org', function(status) {
  if (status !== 'success') {
    console.log('Unable to access network');
  } else {
    var ua = page.evaluate(function() {
      return document.getElementById('myagent').textContent;
    });
    console.log(ua);
  }
  phantom.exit();
});
```
The above example also demonstrates a way to customize the user agent seen by the remote web server.

## Use jQuery and Other Libraries

As of version 1.6, you are also able to include jQuery into your page using a page.includeJs as follows:

```javascript
var page = require('webpage').create();
page.open('http://www.sample.com', function() {
  page.includeJs("http://ajax.googleapis.com/ajax/libs/jquery/1.6.1/jquery.min.js", function() {
    page.evaluate(function() {
      $("button").click();
    });
    phantom.exit()
  });
});
```

The above snippet will open up a web page, include the jQuery library into the page, and then click on all buttons using jQuery. It will then exit from the web page. Make sure to put the exit statement within the page.includeJs or else it may exit prematurely before the javascript code is included.
