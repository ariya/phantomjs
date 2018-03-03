---
layout: post
title:  isReadable
categories: api fs fs-method
permalink: api/fs/method/is-readable.html
---

'isReadable(string)' (BOOL)

This will return true if the file is readable, otherwise it will return false.

This [wikipedia article](http://en.wikipedia.org/wiki/File_system_permissions#Permissions) explains permissions on Unix-like systems.

## Examples

```javascript
var fs = require('fs');
var path = '/Full/Path/To/file';

if (fs.isReadable(path)) {
  console.log('"'+path+'" is readable.');

  // Open the file
  var otherFile = fs.open(path, {
    mode: 'r' //Read mode
  });
  // Do something with the opened file
}
else
  console.log('"'+path+'" is NOT readable.');

phantom.exit();
```








