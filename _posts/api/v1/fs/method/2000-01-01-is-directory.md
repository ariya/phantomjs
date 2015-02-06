---
layout: post
title:  isDirectory
categories: api_v1 fs fs-method
permalink: api/v1/fs/method/is-directory.html
---
Will determine if the path (which is relative to the currtent working directory) is a directory.  Returns a boolean, true if the path is a directory.
## Examples

```javascript
var fs = require('fs');
var path = "/etc/";
// Get a list all files in directory
var list = fs.list(path);
// Cycle through the list
for(var x = 0; x < list.length; x++){
  // Note: If you didn't end path with a slash, you need to do so here.
	var file = path + list[x]; 
	if(fs.isDirectory(file)){
		// Do something
	}
}
```








