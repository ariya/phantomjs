---
layout: post
title:  cookies
section: phantom
kind: property
permalink: api/phantom/property/cookies.html
---

`phantom.cookies` {Object[]}

**Introduced:** PhantomJS 1.7

Get or set Cookies for any domain (though, for setting, use of [`phantom.addCookie`]({{ site.url }}/api/phantom/method/add-cookie.html) is preferred). These Cookies are stored in the CookieJar and will be supplied when opening pertinent WebPages.

This array will be pre-populated by any existing Cookie data stored in the cookie file specified in the PhantomJS [startup config/command-line options]({{ site.url }}/api/command-line.html), if any.

## Examples

```javascript
phantom.cookies
// @TODO: Finish phantom.cookies example.
```








