---
layout: post
title:  viewportSize
categories: api webpage webpage-property
permalink: api/webpage/property/viewport-size.html
---

`viewportSize` {object}

This property sets the size of the viewport for the layout process. It is useful to set the preferred initial size before loading the page, e.g. to choose between `'landscape'` vs `'portrait'`.

Because PhantomJS is headless (nothing is shown), `viewportSize` effectively simulates the size of the window like in a traditional browser.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.viewportSize = {
  width: 480,
  height: 800
};
```
You must include height for this to work.

```javascript
var page = require('webpage').create(),
	address, output, size;

if (phantom.args.length < 4 || phantom.args.length > 5) {
    console.log('Usage: viewport.js URL filename sizeX sizeY');
    phantom.exit();
} else {
    address = phantom.args[0];
    output = phantom.args[1];
    sizeX = phantom.args[2];
    sizeY = phantom.args[3];
    page.viewportSize = { width: sizeX, height: sizeY };
    page.open(address, function (status) {
        if (status !== 'success') {
            console.log('Unable to load the address!');
        } else {
            window.setTimeout(function () {
                page.render(output);
                phantom.exit();
            }, 200);
        }
    });
}
```
Usage: viewport.js URL filename sizeX sizeY






