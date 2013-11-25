---
layout: post
title:  addCookie
categories: api webpage webpage-method
permalink: api/webpage/method/add-cookie.html
---

`addCookie(Cookie)` {boolean}

**Introduced:** PhantomJS 1.7

Add a Cookie to the page. If the domains do not match, the Cookie will be ignored/rejected. Returns `true` if successfully added, otherwise `false`.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.addCookie({
  'name'     : 'Valid-Cookie-Name',   /* required property */
  'value'    : 'Valid-Cookie-Value',  /* required property */
  'domain'   : 'localhost',           /* required property */
  'path'     : '/foo',
  'httponly' : true,
  'secure'   : false,
  'expires'  : (new Date()).getTime() + (1000 * 60 * 60)   /* <-- expires in 1 hour */
});
```








