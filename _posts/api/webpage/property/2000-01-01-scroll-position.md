---
layout: post
title:  scrollPosition
categories: api webpage webpage-property
permalink: api/webpage/property/scroll-position.html
---

`scrollPosition` {object}

This property defines the scroll position of the web page.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.scrollPosition = {
  top: 100,
  left: 0
};
```








