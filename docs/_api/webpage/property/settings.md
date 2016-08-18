---
layout: post
title:  settings
section: webpage
kind: property
permalink: api/webpage/property/settings.html
---

`settings` {object}

This property stores various settings of the web page:

* `javascriptEnabled` defines whether to execute the script in the page or not (defaults to `true`).
* `loadImages` defines whether to load the inlined images or not (defaults to `true`).
* `localToRemoteUrlAccessEnabled` defines whether local resource (e.g. from file) can access remote URLs or not (defaults to `false`).
* `userAgent` defines the user agent sent to server when the web page requests resources.
* `userName` sets the user name used for HTTP authentication.
* `password` sets the password used for HTTP authentication.
* `XSSAuditingEnabled` defines whether load requests should be monitored for cross-site scripting attempts (defaults to `false`).
* `webSecurityEnabled` defines whether web security should be enabled or not (defaults to `true`).
* `resourceTimeout` (in milli-secs) defines the timeout after which any resource requested will stop trying and proceed with other parts of the page. `onResourceTimeout` callback will be called on timeout.

**Note:** The `settings` apply only during the initial call to the `page.open` function. Subsequent modification of the `settings` object will not have any impact.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();
page.settings.userAgent = 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/37.0.2062.120 Safari/537.36';
```






