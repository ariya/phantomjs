---
layout: post
title:  libraryPath
categories: api_v1 webpage webpage-property
permalink: api/v1/webpage/property/library-path.html
---

`libraryPath` {string}

This property stores the path which is used by `page.injectJs` function to resolve the script name.

Initially it is set to the location of the script invoked by PhantomJS.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();
// @TODO: Finish page.libraryPath example.
```








