---
layout: post
title:  onResourceRequested
section: webpage
kind: handler
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
* `changeUrl(newUrl)` : changes the current URL of the network request. By calling `networkRequest.changeUrl(newUrl)`, we can change the request url to the new url. This is an excellent and only way to provide alternative implementation of a remote resource. (see Example-2)
* `setHeader(key, value)`

## Examples

### Example-1
```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onResourceRequested = function(requestData, networkRequest) {
  console.log('Request (#' + requestData.id + '): ' + JSON.stringify(requestData));
};
```

### Example-2

Provide an alternative implementation of a remote javascript.

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onResourceRequested = function(requestData, networkRequest) {
  var match = requestData.url.match(/wordfamily.js/g);
  if (match != null) {
    console.log('Request (#' + requestData.id + '): ' + JSON.stringify(requestData));
    
    // newWordFamily.js is an alternative implementation of wordFamily.js
    // and is available in local path
    networkRequest.changeUrl('newWordFamily.js'); 
  }
};
```







