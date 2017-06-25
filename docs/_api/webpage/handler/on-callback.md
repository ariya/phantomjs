---
layout: post
title:  onCallback
section: webpage
kind: handler
permalink: api/webpage/handler/on-callback.html
---

**Stability:** _EXPERIMENTAL_

**Introduced:** PhantomJS 1.6
This callback is invoked when there is a JavaScript `window.callPhantom` call made on the web page. The only argument passed to the callback is a data object.

**Note**: `window.callPhantom` is still an experimental API. In the near future, it will be likely replaced with a message-based solution which will still provide the same functionality.

Although there are many possible use cases for this inversion of control, the primary one so far is to prevent the need for a PhantomJS script to be continually polling for some variable on the web page.

## Examples

### Send data from client to server

#### WebPage (client-side)

```javascript
if (typeof window.callPhantom === 'function') {
  window.callPhantom({ hello: 'world' });
}
```

#### PhantomJS (server-side)

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onCallback = function(data) {
  console.log('CALLBACK: ' + JSON.stringify(data));
  // Prints 'CALLBACK: { "hello": "world" }'
};
```

### Send data from client to server then back again

#### WebPage (client-side)

```javascript
if (typeof window.callPhantom === 'function') {
  var status = window.callPhantom({
    secret: 'ghostly'
  });
  alert(status);
  // Prints either 'Accepted.' or 'DENIED!'
}
```

#### PhantomJS (server-side)

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onCallback = function(data) {
  if (data && data.secret && (data.secret === 'ghostly')) {
    return 'Accepted.';
  }
  return 'DENIED!';
};
```

### Allow web page Javascript to close PhantomJS

This can be helpful when the web page running in PhantomJS needs to exit PhantomJS.

#### WebPage (client-side)

```javascript
if (typeof window.callPhantom === 'function') {
  var status = window.callPhantom({
    command: 'exit',
    reason:  'User Request.'
  });
}
```

#### PhantomJS (server-side)

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onCallback = function(data) {
  if (data && data.command && (data.command === 'exit')) {
   if (data.reason) console.log('web page requested exit: '+data.reason);
    phantom.exit(0);
  }
};
```







