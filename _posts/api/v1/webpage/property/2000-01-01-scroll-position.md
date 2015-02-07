---
layout: post
title:  scrollPosition
categories: api_v1 webpage webpage-property
permalink: api/v1/webpage/property/scroll-position.html
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








