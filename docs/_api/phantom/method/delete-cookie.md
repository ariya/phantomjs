---
layout: post
title:  deleteCookie
section: phantom
kind: method
permalink: api/phantom/method/delete-cookie.html
---

`deleteCookie(cookieName)` {Boolean}

**Introduced:** PhantomJS 1.7

Delete any Cookies in the CookieJar with a '`name'` property matching `cookieName`. Returns `true` if successfully deleted, otherwise `false`.

## Examples

```javascript
phantom.deleteCookie('Added-Cookie-Name');
```








