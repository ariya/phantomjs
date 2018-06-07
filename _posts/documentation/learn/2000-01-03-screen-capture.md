---
layout: post
title: Screen Capture
categories: docs docs-learn
permalink: screen-capture.html
---

Since PhantomJS is using WebKit, a real layout and rendering engine, it can capture a web page as a screenshot. Because PhantomJS can render anything on the web page, it can be used to convert HTML content styled with CSS but also SVG, images and Canvas elements.

The following script demonstrates the simplest use of page capture. It loads the GitHub homepage and then saves it as an image, `github.png`.

```javascript
var page = require('webpage').create();
page.open('http://github.com/', function() {
  page.render('github.png');
  phantom.exit();
});
```

To run this example create a new file called `github.js`. Copy and paste the above code into the `github.js` file. In the commandline, run this newly created script with PhantomJS:

```bash
phantomjs github.js
```

Beside PNG format, PhantomJS supports JPEG, GIF, and PDF.

In the `examples` subdirectory, there is a script [rasterize.js](https://github.com/ariya/phantomjs/blob/master/examples/rasterize.js) which demonstrates a more complete rendering feature of PhantomJS. An example to produce the rendering of the famous Tiger (from SVG):

```bash
phantomjs rasterize.js http://ariya.github.io/svg/tiger.svg tiger.png
```
which gives the following `tiger.png`:

![Rendered Tiger](http://lh6.ggpht.com/_Oijhf1ZPv-4/TR6iM8J0KrI/AAAAAAAABy4/RCZ8Eg567LM/s400/tiger.png)

If you need to add a header and a footer including page numbers in your PDF output, use [printheaderfooter.js](https://github.com/ariya/phantomjs/blob/master/examples/printheaderfooter.js). 

```bash
phantomjs printheaderfooter.js http://phantomjs.org/screen-capture.html screenCapture.pdf
```


Another example is to show [polar clock](https://dmitrybaranovskiy.github.io/raphael/polar-clock.html) (from [RaphaelJS](https://dmitrybaranovskiy.github.io/raphael/)):

```bash
phantomjs rasterize.js https://dmitrybaranovskiy.github.io/raphael/polar-clock.html clock.png
```

![Polar Clock](https://lh5.googleusercontent.com/_Oijhf1ZPv-4/TUuUx1o-tuI/AAAAAAAAB00/Ba-Gxl5Zp6Q/s288/polar-clock.png)

Producing PDF output is also easy, such as from a Wikipedia article:

```bash
phantomjs rasterize.js 'http://en.wikipedia.org/w/index.php?title=Jakarta&printable=yes' jakarta.pdf
```

You can change the size of the screenshot and the webpage using the page's attributes:

```javascript
var page = require('webpage').create();
//viewportSize being the actual size of the headless browser
page.viewportSize = { width: 1024, height: 768 };
//the clipRect is the portion of the page you are taking a screenshot of
page.clipRect = { top: 0, left: 0, width: 1024, height: 768 };
//the rest of the code is the same as the previous example
page.open('http://example.com/', function() {
  page.render('github.png');
  phantom.exit();
});
```


Canvas can be easily constructed and converted to an image. The included example [colorwheel.js](https://github.com/ariya/phantomjs/blob/master/examples/colorwheel.js) produces the following color wheel:

![Color Wheel](https://lh3.googleusercontent.com/-xSIzxPtJULw/TVzeP4NPMDI/AAAAAAAAB10/k-c8jB6I5Cg/s288/colorwheel.png)

It is possible to build a web screenshot service using PhantomJS. Some [related projects]({{ site.url }}/related-projects.html) make it easy to create such a service.
