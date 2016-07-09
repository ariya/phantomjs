---
layout: post
title:  evaluateAsync
categories: api webpage webpage-method
permalink: api/webpage/method/evaluate-async.html
---

`evaluateAsync(function, [delayMillis, arg1, arg2, ...])` {void}

Evaluates the given function in the context of the web page, without blocking the current execution. The function returns immediately and there is no return value. This is useful to run some script asynchronously.

The second argument indicates the time (in milliseconds) before the function should execute. The remaining arguments are passed to the function, as with [`evaluate`](http://phantomjs.org/api/webpage/method/evaluate.html). You must specify a delay (which can be `0`) if you want to pass in any arguments.

## Examples

### Asynchronous AJAX

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.open("", function(status) {
  page.includeJs("http://ajax.googleapis.com/ajax/libs/jquery/1.6.1/jquery.min.js", function() {

    page.evaluateAsync(function() {
      $.ajax({url: "api1", success: function() {}});
    });

    page.evaluateAsync(function(apiUrl) {
      $.ajax({url: apiUrl, success: function() {}});
    }, 1000, "api2");

  });
});
```

