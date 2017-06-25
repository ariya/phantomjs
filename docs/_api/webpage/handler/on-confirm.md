---
layout: post
title:  onConfirm
section: webpage
kind: handler
permalink: api/webpage/handler/on-confirm.html
---

**Introduced:** PhantomJS 1.6

This callback is invoked when there is a JavaScript `confirm` on the web page. The only argument passed to the callback is the string for the message. The return value of the callback handler can be either `true` or `false`, which are equivalent to pressing the "OK" or "Cancel" buttons presented in a JavaScript `confirm`, respectively.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onConfirm = function(msg) {
  console.log('CONFIRM: ' + msg);
  return true; // `true` === pressing the "OK" button, `false` === pressing the "Cancel" button
};
```








