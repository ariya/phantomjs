---
layout: post
title:  onLoadStarted
categories: api webpage webpage-handler
permalink: api/webpage/handler/on-load-started.html
---

**Introduced:** PhantomJS 1.2

This callback is invoked when the page starts the loading. There is no argument passed to the callback.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onLoadStarted = function() {
  var currentUrl = page.evaluate(function() {
    return window.location.href;
  });
  console.log('Current page ' + currentUrl + ' will gone...');
  console.log('Now loading a new page...');
};
```








