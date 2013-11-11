---
layout: post
title:  onResourceRequested
categories: api webpage webpage-handler
permalink: api/webpage/handler/on-resource-requested.html
---

**Introduced:** PhantomJS 1.2

This callback is invoked when the page requests a resource. The first argument to the callback is the `requestData` metadata object. The second argument is the `networkRequest` object itself.

The `requestData` metadata object contains these properties:

* `id`      : the number of the requested resource
* `method`  : http method
* `url`     : the URL of the requested resource
* `time`    : Date object containing the date of the request
* `headers` : list of http headers

The `networkRequest` object contains these functions:

* `abort()`        : aborts the current network request. Aborting the current network request will invoke `onResourceError` callback.
* `changeUrl(url)` : changes the current URL of the network request.
* `setHeader(key, value)`

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onResourceRequested = function(requestData, networkRequest) {
  console.log('Request (#' + requestData.id + '): ' + JSON.stringify(requestData));
};
```








