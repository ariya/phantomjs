---
layout: post
title:  onUrlChanged
categories: api_v1 webpage webpage-handler
permalink: api/v1/webpage/handler/on-url-changed.html
---

**Introduced:** PhantomJS 1.6

This callback is invoked when the URL changes, e.g. as it navigates away from the current URL. The only argument to the callback is the new `targetUrl` string.

To retrieve the old URL, use the onLoadStarted callback.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onUrlChanged = function(targetUrl) {
  console.log('New URL: ' + targetUrl);
};
```








