---
layout: post
title:  onFilePicker
categories: api webpage webpage-handler
permalink: api/webpage/handler/on-file-picker.html
---

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();
var system = require('system');

page.onFilePicker = function(oldFile) {
  if (system.os.name === 'windows') {
    return 'C:\\Windows\\System32\\drivers\\etc\\hosts';
  }
  return '/etc/hosts';
};
```
