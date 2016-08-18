---
layout: post
title:  onAlert
section: webpage
kind: handler
permalink: api/webpage/handler/on-alert.html
---

**Introduced:** PhantomJS 1.0

This callback is invoked when there is a JavaScript `alert` on the web page. The only argument passed to the callback is the string for the message. There is no return value expected from the callback handler.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onAlert = function(msg) {
  console.log('ALERT: ' + msg);
};
```








