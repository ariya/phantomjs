---
layout: post
title:  makeDirectory
categories: api fs fs-method
permalink: api/fs/method/make-directory.html
---

'makeDirectory(string)' (BOOL)

Creates a directory at the given path.

This will returns true if the creation was successful, otherwise false. If the directory already exists, it will returns false.

## Examples

```javascript
var fs = require('fs');

var path = '/Full/Path/To/Test/';

if(fs.makeDirectory(path))
  console.log('"'+path+'" was created.');
else
  console.log('"'+path+'" is NOT created.');

phantom.exit();
```








