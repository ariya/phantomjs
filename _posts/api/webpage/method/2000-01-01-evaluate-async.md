---
layout: post
title:  evaluateAsync
categories: api webpage webpage-method
permalink: api/webpage/method/evaluate-async.html
---

`evaluateAsync(function)` {void}

Evaluates the given function in the context of the web page without blocking the current execution. The function returns immediately and there is no return value. This is useful to run some script asynchronously.

## Examples
###Asynchronous AJAX
```javascript
var webPage = require('webpage');
var page = webPage.create();

page.open("", function(status) {
  page.includeJs("http://ajax.googleapis.com/ajax/libs/jquery/1.6.1/jquery.min.js", function() {

    page.evaluateAsync(function() {
      $.ajax({url: "api1", success: function() {}});
    });

    page.evaluateAsync(function() {
      $.ajax({url: "api2", success: function() {}});
    });

  });
});
```
