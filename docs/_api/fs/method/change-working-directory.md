---
layout: post
title:  changeWorkingDirectory
section: fs
kind: method
permalink: api/fs/method/change-working-directory.html
---

'changeWorkingDirectory(string)' (BOOL)

This allows you to change the workingDirectory and returns true if the change happened else false

## Examples

```javascript
var fs = require('fs');
console.log(fs.workingDirectory); //prints the location where phantomjs is running
fs.changeWorkingDirectory("C:\\");
console.log(fs.workingDirectory); //prints C:/
```








