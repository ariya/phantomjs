---
layout: post
title:  content
section: webpage
kind: property
permalink: api/webpage/property/content.html
---

`content` {string}

This property stores the content of the web page (main frame), enclosed in an HTML/XML element. Setting the property will effectively reload the web page with the new content.

See also `page.plainText` to get the content without any HTML tags.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.open('http://phantomjs.org', function (status) {
  var content = page.content;
  console.log('Content: ' + content);
  phantom.exit();
});
```








