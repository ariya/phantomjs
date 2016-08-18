---
layout: post
title:  write
section: stream
kind: method
permalink: api/stream/method/write.html
---

## Stream objects

Stream objects are returned from the [fs.open]({{ site.url }}/api/fs/method/open.html) method.

## Examples

```javascript
var fs = require('fs');
var stream = fs.open('output.txt', 'w');

stream.write('Hello, ');
stream.write('world!\n');
stream.close();

stream = fs.open('output.txt', 'r');
console.log(stream.readLine());
stream.close();
phantom.exit();
```








