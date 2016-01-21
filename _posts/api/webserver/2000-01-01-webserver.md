---
layout: post
title: Web Server Module
categories: api webserver
permalink: api/webserver/index.html
---

**Stability:** _EXPERIMENTAL_

**Introduced:** PhantomJS 1.4

Using an embedded web server module called [Mongoose](https://github.com/cesanta/mongoose), PhantomJS script can start a web server. This is intended for ease of communication between PhantomJS scripts and the outside world and is _not_ recommended for use as a general production server. There is currently a limit of **10** concurrent requests; any other requests will be queued up.

To start using, you must `require` a reference to the `webserver` module:

```javascript
var webserver = require('webserver');
```
