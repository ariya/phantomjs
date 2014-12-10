---
layout: post
title:  paperSize
categories: api webpage webpage-property
permalink: api/webpage/property/paper-size.html
---

`paperSize` {object}

This property defines the size of the web page when rendered as a PDF.

## Values and measurements

### paperSize

If no `paperSize` is defined, the size is defined by the web page.

Supported dimension units are: `'mm'`, `'cm'`, `'in'`, `'px'`. No unit means `'px'`.

### Margin

(*Previously known as __Border__, which can still be used for compatibility purposes*)

Margin is optional and defaults to `0`.  Can be provided as a single dimension or as an object containing one or more of the following properties: `'top'`, `'left'`, `'bottom'`, `'right'`.

### Format

Supported formats are: `'A3'`, `'A4'`, `'A5'`, `'Legal'`, `'Letter'`, `'Tabloid'`.

### Orientation

Orientation (`'portrait'`, `'landscape'`) is optional and defaults to `'portrait'`.

### Headers and Footers

A repeating page header and footer can also be added via the header and footer property. These can be provided as an object that contains a `height` and a `contents` property. The contents property must be set as a phantom.callback object. 

```javascript
header: {
  height: "1.2cm",
  contents: phantom.callback(function(pageNum, numPages) {
    return "<h1>Header <span style='float:right'>" + pageNum + " / " + numPages + "</span></h1>";
  })
}
```


## Acceptable Objects

The given object should be in _either_ this format:

```javascript
{
  width: '200px',
  height: '300px',
  margin: '0px'
}
```

or this format:

```javascript
{
  format: 'A4',
  orientation: 'portrait',
  margin: '1cm'
}
```

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.paperSize = {
  width: '5in',
  height: '7in',
  margin: {
    top: '50px',
    left: '20px'
  }
};
```

A repeating page `header` and `footer` can also be added via this property, as in [this example](https://github.com/ariya/phantomjs/blob/master/examples/printheaderfooter.js):

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.paperSize = {
  width: '8.5in',
  height: '11in',
  header: {
    height: "1cm",
    contents: phantom.callback(function(pageNum, numPages) {
      return "<h1>Header <span style='float:right'>" + pageNum + " / " + numPages + "</span></h1>";
    })
  },
  footer: {
    height: "1cm",
    contents: phantom.callback(function(pageNum, numPages) {
      return "<h1>Footer <span style='float:right'>" + pageNum + " / " + numPages + "</span></h1>";
    })
  }
}
```








