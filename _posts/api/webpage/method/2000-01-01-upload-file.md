---
layout: post
title:  uploadFile
categories: api webpage webpage-method
permalink: api/webpage/method/upload-file.html
---

`uploadFile(selector, filename)`

Uploads the specified file (`filename`) to the form element associated with the `selector`.

This function is used to automate the upload of a file, which is usually handled with a file dialog in a traditional browser. Since there is no dialog in this headless mode, such an upload mechanism is handled via this special function instead.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.uploadFile('input[name=image]', '/path/to/some/photo.jpg');
```








