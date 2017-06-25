---
layout: post
title:  onFilePicker
section: webpage
kind: handler
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
