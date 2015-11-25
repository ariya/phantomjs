---
layout: post
title:  includeJs
categories: api webpage webpage-method
permalink: api/webpage/method/include-js.html
---

`includeJs(url, callback)` {void}

Includes external script from the specified `url` (usually a remote location) on the page and executes the `callback` upon completion.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.includeJs(
  // Include the https version, you can change this to http if you like.
  'https://ajax.googleapis.com/ajax/libs/jquery/1.8.2/jquery.min.js',
  function() {
    (page.evaluate(function() {
      // jQuery is loaded, now manipulate the DOM
      var $loginForm = $('form#login');
      $loginForm.find('input[name="username"]').value('phantomjs');
      $loginForm.find('input[name="password"]').value('c45p3r');
    }))
  }
);


```








