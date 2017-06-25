---
layout: post
title:  render
section: webpage
kind: method
permalink: api/webpage/method/render.html
---

`render(filename [, {format, quality}])` {void}

Renders the web page to an image buffer and saves it as the specified `filename`.

Currently, the output format is automatically set based on the file extension.

## Supported formats

* PDF
* PNG
* JPEG
* BMP
* PPM
* GIF support depends on the build of Qt used

## Quality

An integer between 0 and 100. 

The quality setting only has an effect on jpeg and png formats. With jpeg, it sets the quality level as a percentage, in the same way as most image editors. (The output file always has 2x2 subsampling.) A level of 0 produces a very small, very low quality file, and 100 produces a much larger, high-quality file. The default level is 75. With png, it sets the lossless (Deflate) compression level, with 0 producing the smallest files, and 100 producing the largest. However, the files look identical, and are always true-colour.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.viewportSize = { width: 1920, height: 1080 };
page.open("http://www.google.com", function start(status) {
  page.render('google_home.jpeg', {format: 'jpeg', quality: '100'});
  phantom.exit();
});
```

## More information

The image generation code (except for PDF output) uses QImage from the Qt framework, documented at http://doc.qt.io/qt-5/qimage.html#save.






