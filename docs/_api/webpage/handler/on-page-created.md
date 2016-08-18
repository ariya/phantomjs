---
layout: post
title:  onPageCreated
section: webpage
kind: handler
permalink: api/webpage/handler/on-page-created.html
---

**Introduced:** PhantomJS 1.7

This callback is invoked when a new child window (but _not_ deeper descendant windows) is created by the page, e.g. using [`window.open`](https://developer.mozilla.org/docs/DOM/window.open).

In the PhantomJS outer space, this `WebPage` object will not yet have called its own `page.open` method yet and thus does not yet know its requested URL (`page.url`).

Therefore, the most common purpose for utilizing a `page.onPageCreated` callback is to decorate the page (e.g. hook up callbacks, etc.).

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onPageCreated = function(newPage) {
  console.log('A new child page was created! Its requested URL is not yet available, though.');
  // Decorate
  newPage.onClosing = function(closingPage) {
    console.log('A child page is closing: ' + closingPage.url);
  };
};
```








