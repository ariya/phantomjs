---
layout: post
title:  addCookie
section: webpage
kind: method
permalink: api/webpage/method/add-cookie.html
---

`addCookie(Cookie)` {boolean}

**Introduced:** PhantomJS 1.7

Add a Cookie to the page. If the `domain` does not match the current page, the Cookie will be ignored/rejected. Returns `true` if successfully added, otherwise `false`.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

phantom.addCookie({
  'name'     : 'Valid-Cookie-Name',   /* required property */
  'value'    : 'Valid-Cookie-Value',  /* required property */
  'domain'   : 'localhost',
  'path'     : '/foo',                /* required property */
  'httponly' : true,
  'secure'   : false,
  'expires'  : (new Date()).getTime() + (1000 * 60 * 60)   /* <-- expires in 1 hour */
});
```








