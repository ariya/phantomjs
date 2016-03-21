---
layout: post
title:  writeLine
categories: api fs stream stream-method
permalink: api/stream/method/write-line.html
---

## Stream objects

Stream objects are returned from the [fs.open]({{ site.url }}/api/fs/method/open.html) method.

## Examples

```javascript
var fs = require('fs');
var stream = fs.open('output.txt', 'w');

stream.writeLine('Hello');
stream.writeLine('world!');
stream.close();

stream = fs.open('output.txt', 'r');
console.log(stream.read());
stream.close();
phantom.exit();
```








