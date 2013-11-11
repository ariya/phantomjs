---
layout: post
title:  onLoadFinished
categories: api webpage webpage-handler
permalink: api/webpage/handler/on-load-finished.html
---

**Introduced:** PhantomJS 1.2

This callback is invoked when the page finishes the loading. It may accept a single argument indicating the page's `status`: `'success'` if no network errors occurred, otherwise `'fail'`.

Also see `page.open` for an alternate hook for the `onLoadFinished` callback.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onLoadFinished = function(status) {
  console.log('Status: ' + status);
  // Do other things here...
};
```








