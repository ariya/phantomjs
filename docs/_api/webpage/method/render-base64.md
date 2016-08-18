---
layout: post
title:  renderBase64
section: webpage
kind: method
permalink: api/webpage/method/render-base64.html
---

`renderBase64(format)`

Renders the web page to an image buffer and returns the result as a [Base64](http://en.wikipedia.org/wiki/Base64)-encoded string representation of that image.

## Supported formats

* PNG
* GIF
* JPEG

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.viewportSize = {
	width: 1920,
	height: 1080
};

page.open('http://phantomjs.org', function (status) {
  var base64 = page.renderBase64('PNG');
  console.log(base64);
  phantom.exit();
});
```








