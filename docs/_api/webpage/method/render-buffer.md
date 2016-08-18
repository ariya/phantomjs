---
layout: post
title:  renderBuffer
section: webpage
kind: method
permalink: api/webpage/method/render-buffer.html
---

`renderBuffer(format, quality)`

Renders the web page to an image buffer which can be sent directly to a client (e.g. using the webserve module)

## Supported formats

* PNG
* GIF
* JPEG

## Examples

```javascript
var server = require('webserver').create();

var listening = server.listen(8001, function(request, response) {
	var url = "http://phantomjs.org", format = 'png', quality = -1;

	var page = require('webpage').create();

	page.viewportSize = {
		width: 800,
		height: 600
	};

	page.open(url, function start(status) {

		// Buffer is an Uint8ClampedArray
		var buffer = page.renderBuffer(format, quality);

		response.statusCode = 200;
		response.headers = {
			"Cache": "no-cache",
			"Content-Type": "image/" + format
		};
		page.close();

		// Pass the Buffer to 'write' to send the Uint8ClampedArray to the client
		response.write(buffer);
		response.close();
	});

});
```

