---
layout: post
title:  removeTree
categories: api fs fs-method
permalink: api/fs/method/remove-tree.html
---

'removeTree(string)'

This will try to delete every file and folder in the specified folder and, finally, delete the folder itself.

When errors occur during a call, it will throw a 'Unable to remove directory tree PATH' and hang execution.

## Examples

```javascript
var fs = require('fs');
var toDelete = 'someFolder';

fs.makeDirectory(toDelete);
fs.touch(toDelete + '/test.txt');

// It will delete the 'someFolder' and the 'test.txt' in it
fs.removeTree(toDelete);

phantom.exit();
```








