---
layout: post
title:  isAbsolute
categories: api_v1 fs fs-method
permalink: api/v1/fs/method/is-absolute.html
---

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








