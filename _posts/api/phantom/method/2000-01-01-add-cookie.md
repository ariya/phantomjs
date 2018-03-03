---
layout: post
title:  addCookie
categories: api phantom phantom-method
permalink: api/phantom/method/add-cookie.html
---

`addCookie(Object)` {Boolean}

**Introduced:** PhantomJS 1.7

Add a Cookie to the CookieJar.  Returns `true` if successfully added, otherwise `false`.

## Examples

```javascript
phantom.addCookie({
  name: 'Added-Cookie-Name',
  value: 'Added-Cookie-Value',
  domain: '.google.com'
});
```








