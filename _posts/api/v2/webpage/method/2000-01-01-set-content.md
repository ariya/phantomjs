---
layout: post
title:  setContent
categories: api_v2 webpage webpage-method
permalink: api/v2/webpage/method/set-content.html
---

**Introduced:** PhantomJS 1.8

Allows to set both `page.content` and `page.url` properties.

The webpage will be reloaded with the new content and the current location set as the given url, without any actual http request being made.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();
// @TODO: Finish page.setContent example.
```








