---
layout: post
title:  makeDirectory
categories: api_v1 fs fs-method
permalink: api/v1/fs/method/make-directory.html
---

// @TODO: Finish fs.makeDirectory parameter documentation.

## Examples

```javascript
var fs = require('fs');

var wasSuccessful = fs.makeDirectory('test');

phantom.exit((wasSuccessful === true ? 0 : 1));
```

