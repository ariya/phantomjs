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

### Border

Border is optional and defaults to `0`.

Supported formats are: `'A3'`, `'A4'`, `'A5'`, `'Legal'`, `'Letter'`, `'Tabloid'`.

### Orientation

Orientation (`'portrait'`, `'landscape'`) is optional and defaults to `'portrait'`.

## Acceptable Objects

The given object should be in _either_ this format:

```javascript
{
  width: '200px',
  height: '300px',
  border: '0px'
}
```

or this format:

```javascript
{
  format: 'A4',
  orientation: 'portrait',
  border: '1cm'
}
```

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.paperSize = {
  width: '5in',
  height: '7in',
  border: '20px'
};
```








