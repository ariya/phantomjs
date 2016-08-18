---
layout: post
title:  onInitialized
section: webpage
kind: handler
permalink: api/webpage/handler/on-initialized.html
---

**Introduced:** PhantomJS 1.3

This callback is invoked _after_ the web page is created but _before_ a URL is loaded. The callback may be used to change global objects.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onInitialized = function() {
  page.evaluate(function() {
    document.addEventListener('DOMContentLoaded', function() {
      console.log('DOM content has loaded.');
    }, false);
  });
};
```








