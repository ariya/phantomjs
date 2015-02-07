---
layout: post
title:  zoomFactor
categories: api_v1 webpage webpage-property
permalink: api/v1/webpage/property/zoom-factor.html
---

`zoomFactor` {number}

This property specifies the scaling factor for the `page.render` and `page.renderBase64` functions.

The default is `1`, i.e. 100% zoom.

## Examples

### Create a thumbnail preview with 25% zoom

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.zoomFactor = 0.25;
page.render('capture.png');
```








