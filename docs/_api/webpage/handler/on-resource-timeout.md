---
layout: post
title:  onResourceTimeout
section: webpage
kind: handler
permalink: api/webpage/handler/on-resource-timeout.html
---

**Introduced:** PhantomJS 1.2

This callback is invoked when a resource requested by the page timeout according to `settings.resourceTimeout`. The only argument to the callback is the `request` metadata object.

The `request` metadata object contains these properties:

 * `id`: the number of the requested resource
 * `method`: http method
 * `url`: the URL of the requested resource
 * `time`: Date object containing the date of the request
 * `headers`: list of http headers
 * `errorCode`: the error code of the error
 * `errorString`: text message of the error

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onResourceTimeout = function(request) {
    console.log('Response (#' + request.id + '): ' + JSON.stringify(request));
};
```
