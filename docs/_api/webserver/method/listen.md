---
layout: post
title:  listen
section: webserver
kind: method
permalink: api/webserver/method/listen.html
---

## `request` argument

The `request` object passed to the callback function may contain the following properties:

* `method`: Defines the request method (`'GET'`, `'POST'`, etc.)
* `url`: The path part and query string part (if any) of the request URL
* `httpVersion`: The actual HTTP version
* `headers`: All of the HTTP headers as key-value pairs
* `post`: The request body (only for `'POST'` and `'PUT'` method requests)
* `postRaw`: If the `Content-Type` header is set to `'application/x-www-form-urlencoded'` (the default for form submissions), the original contents of `post` will be stored in this extra property (`postRaw`) and then `post` will be automatically updated with a URL-decoded version of the data.

## `response` argument

The `response` object should be used to create the response using the following properties and functions:

* `headers`: Stores all HTTP headers as key-value pairs. These must be set **BEFORE** calling `write` for the first time.
* `setHeader(name, value)`: sets a specific header
* `header(name)`: returns the value of the given header
* `statusCode`: Sets the returned HTTP status code.
* `setEncoding(encoding)`: Indicate the encoding to use to convert data given to `write()`. By default data will be converted to UTF-8. Indicate `"binary"` if data is a binary string. Not required if data is a Buffer (e.g. from page.renderBuffer).
* `write(data)`: Sends a chunk for the response body. Can be called multiple times.
* `writeHead(statusCode, headers)`: Sends a response header to the request. The status code is a 3-digit HTTP status code (e.g. `404`). The last argument, headers, are the response headers. Optionally one can give a human-readable `headers` collection as the second argument.
* `close()`: Closes the HTTP connection.
  * To avoid the client detecting a connection drop, remember to use `write()` at least once. Sending an empty string (i.e. `response.write('')`) would be enough if the only aim is, for example, to return an HTTP status code of `200` (`"OK"`).
* `closeGracefully()`: same as `close()`, but ensures response headers have been sent first (and do at least a `response.write('')`)

## Examples

Here is a simple example that always gives the same response regardless of the request:

```javascript
var webserver = require('webserver');
var server = webserver.create();
var service = server.listen(8080, function(request, response) {
  response.statusCode = 200;
  response.write('<html><body>Hello!</body></html>');
  response.close();
});
```

If you want to bind to specify address, just use `ipaddress:port` instead of `port`.

```javascript
var webserver = require('webserver');
var server = webserver.create();
var service = server.listen('127.0.0.1:8080', function(request, response) {
  response.statusCode = 200;
  response.write('<html><body>Hello!</body></html>');
  response.close();
});
```

The value returned by `server.listen()` is a boolean: true if the server is launched.

An optional object `opts` can be passed between the IP/port and the callback:

```javascript
var service = server.listen(8080, {
  'keepAlive': true
}, function(request, response) {

});
```

Currently, the only supported option is `keepAlive`. If set to `true`, the webserver will support keep-alive connections. Note: servers that have keep-alive enabled **must** set a proper Content-Length header in their responses, otherwise clients will not know when the response is finished; additionally, `response.close()` must be called. See examples/serverkeepalive.js for more information.








