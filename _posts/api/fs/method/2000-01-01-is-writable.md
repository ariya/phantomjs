---
layout: post
title:  isWritable
categories: api fs fs-method
permalink: api/fs/method/is-writable.html
---

'isWritable(string)' (BOOL)

This will return true if the file is writable, otherwise it will return false.

This [wikipedia article](http://en.wikipedia.org/wiki/File_system_permissions#Permissions) explains permissions on Unix-like systems.

## Examples

```javascript
var fs = require('fs');
var path = '/Full/Path/To/file';

if (fs.isWritable(path)) {
  console.log('"'+path+'" is writable.');

  var content = 'Hello World!';
  fs.write(path, content, 'w');
}
else
  console.log('"'+path+'" is NOT writable.');

phantom.exit();
```








