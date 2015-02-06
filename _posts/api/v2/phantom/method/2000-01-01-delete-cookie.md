---
layout: post
title:  deleteCookie
categories: api_v2 phantom phantom-method
permalink: api/v2/phantom/method/delete-cookie.html
---

`deleteCookie(cookieName)` {Boolean}

**Introduced:** PhantomJS 1.7

Delete any Cookies in the CookieJar with a '`name'` property matching `cookieName`. Returns `true` if successfully deleted, otherwise `false`.

## Examples

```javascript
phantom.deleteCookie('Added-Cookie-Name');
```








