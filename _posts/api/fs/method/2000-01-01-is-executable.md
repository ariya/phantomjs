---
layout: post
title:  isExecutable
categories: api fs fs-method
permalink: api/fs/method/is-executable.html
---

'isExecutable(string)' (BOOL)

This will return true if the file path is executable, otherwise it will return false.

This [wikipedia article](http://en.wikipedia.org/wiki/File_system_permissions#Permissions) explains permissions on Unix-like systems.

## Examples

```javascript
var fs = require('fs');
var path = '/Full/Path/To/exec';

if (fs.isExecutable(path))
  console.log('"'+path+'" is executable.');
else
  console.log('"'+path+'" is NOT executable.');

phantom.exit();
```








