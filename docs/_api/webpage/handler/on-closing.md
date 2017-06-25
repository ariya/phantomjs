---
layout: post
title:  onClosing
section: webpage
kind: handler
permalink: api/webpage/handler/on-closing.html
---

**Introduced:** PhantomJS 1.7

This callback is invoked when the `WebPage` object is being closed, either via `page.close` in the PhantomJS outer space or via [`window.close`](https://developer.mozilla.org/docs/DOM/window.close) in the page's client-side.

It is _not_ invoked when child/descendant pages are being closed unless you also hook them up individually. It takes one argument, `closingPage`, which is a reference to the page that is closing. Once the `onClosing` handler has finished executing (returned), the `WebPage` object `closingPage` will become invalid.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onClosing = function(closingPage) {
  console.log('The page is closing! URL: ' + closingPage.url);
};
```








