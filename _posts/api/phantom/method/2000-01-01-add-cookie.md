---
layout: post
title:  addCookie
categories: api phantom phantom-method
permalink: api/phantom/method/add-cookie.html
---

`addCookie(Object)` {Boolean}

The parameter object can contain the following fields:

 - name: string, cookie name
 - value: string, cookie value
 - domain: string, optional. Must start with ".", even if it's a host-specific cookie
 - path: string, optional
 - httponly: boolean, optional
 - secure: boolean, optional
 - expires: optional, either a number that contains milliseconds since Jan 1, 1970, or a string in format "ddd, dd MMM yyyy hh:mm:ss"
 - expiry: alias for expires. If both are provided, the former takes precedence

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








