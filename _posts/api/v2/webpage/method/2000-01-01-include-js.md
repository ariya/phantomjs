---
layout: post
title:  includeJs
categories: api_v2 webpage webpage-method
permalink: api/v2/webpage/method/include-js.html
---

`includeJs(url, callback)` {void}

Includes external script from the specified `url` (usually a remote location) on the page and executes the `callback` upon completion.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.includeJs('http://ajax.googleapis.com/ajax/libs/jquery/1.8.2/jquery.min.js', function() {
  // jQuery is loaded, now manipulate the DOM
  var $loginForm = $('form#login');
  $loginForm.find('input[name="username"]').value('phantomjs');
  $loginForm.find('input[name="password"]').value('c45p3r');
});
```








