---
layout: post
title:  onResourceReceived
section: webpage
kind: handler
permalink: api/webpage/handler/on-resource-received.html
---

**Introduced:** PhantomJS 1.2

This callback is invoked when a resource requested by the page is received. The only argument to the callback is the `response` metadata object.

If the resource is large and sent by the server in multiple chunks, `onResourceReceived` will be invoked for every chunk received by PhantomJS.

The `response` metadata object contains these properties:

* `id`          : the number of the requested resource
* `url`         : the URL of the requested resource
* `time`        : Date object containing the date of the response
* `headers`     : list of http headers
* `bodySize`    : size of the received content decompressed (entire content or chunk content)
* `contentType` : the content type if specified
* `redirectURL` : if there is a redirection, the redirected URL
* `stage`       : "start", "end" (FIXME: other value for intermediate chunk?)
* `status`      : http status code. ex: `200`
* `statusText`  : http status text. ex: `OK`

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onResourceReceived = function(response) {
  console.log('Response (#' + response.id + ', stage "' + response.stage + '"): ' + JSON.stringify(response));
};
```








