---
layout: post
title:  removeDirectory
categories: api fs fs-method
permalink: api/fs/method/remove-directory.html
---

'removeDirectory(string)'

This will try to delete the specified folder.

The directory needs to be empty to be removed with this method. To delete an non empty folder, [fs.removeTree]({{ site.url }}/api/fs/method/remove-tree.html) should be used.

When errors occur during a call, it will throw a 'Unable to remove directory PATH' and hang execution.

## Examples

```javascript
var fs = require('fs');
var toDelete = 'someFolder';

// Test if the folder is empty before deleting it
if(fs.list(toDelete).length === 0)
    fs.removeDirectory(toDelete);

phantom.exit();
```








