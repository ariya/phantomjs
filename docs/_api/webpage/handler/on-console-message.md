---
layout: post
title:  onConsoleMessage
section: webpage
kind: handler
permalink: api/webpage/handler/on-console-message.html
---

**Introduced:** PhantomJS 1.2

This callback is invoked when there is a JavaScript `console` message on the web page. The callback may accept up to three arguments: the string for the message, the line number, and the source identifier.

By default, `console` messages from the web page are not displayed. Using this callback is a typical way to redirect it.

Note: line number and source identifier are not used yet, at least in phantomJS <= 1.8.1. You receive undefined values.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onConsoleMessage = function(msg, lineNum, sourceId) {
  console.log('CONSOLE: ' + msg + ' (from line #' + lineNum + ' in "' + sourceId + '")');
};
```








