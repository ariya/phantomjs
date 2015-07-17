---
layout: post
title:  setContent
categories: api webpage webpage-method
permalink: api/webpage/method/set-content.html
---

**Introduced:** PhantomJS 1.8

Allows to set both `page.content` and `page.url` properties.

The webpage will be reloaded with the new content and the current location set as the given url, without any actual http request being made.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();
var expectedContent = '<html><body><div>Test div</div></body></html>';
var expectedLocation = 'http://www.phantomjs.org/';
page.setContent(expectedContent, expectedLocation);
```








