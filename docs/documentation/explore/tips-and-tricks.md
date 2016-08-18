---
layout: post
title: Tips and Tricks
categories: docs docs-explore
permalink: tips-and-tricks.html
---

## Page background color

PhantomJS does not set the background color of the web page at all, it is left to the page to decide its background color. If the page does not set anything, then it remains transparent.

To force a certain color for the web page background, use the following code:

```javascript
page.evaluate(function() {
  document.body.bgColor = 'white';
});
```

Make sure this is executed before calling render() function of the web page.

### Use `-webkit-calc()` instead of `calc()`

The version of WebKit used by PhantomJS supports CSS3 `calc()` if you add the `-webkit` prefix:

```scss
iframe {
  height: 100%;
  height: calc(100% - 48px); /* Will not work */
  height: -webkit-calc(100% - 48px); /* Works */
}
```
