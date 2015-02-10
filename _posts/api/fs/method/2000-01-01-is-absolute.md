---
layout: post
title:  isAbsolute
categories: api fs fs-method
permalink: api/fs/method/is-absolute.html
---

'isAbsolute(string)' (BOOL)

This will return true if the file path is absolute, otherwise it will return false if the path is relative.

## Examples

```javascript
var fs = require('fs');
var path = '/Full/Path/To/test.txt';

// isAbsolute(path) returns true if the specified path is an absolute path.
if (fs.isAbsolute(path))
  console.log('"'+path+'" is an absolute path.');
else
  console.log('"'+path+'" is NOT an absolute path.');

phantom.exit();
```








