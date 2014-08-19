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

### Using calc() from CSS3

The version of Webkit used by PhantomJS does not support the CSS3 `calc()` feature. It is good practice to set a fallback for clients that do not support it. Your end users might not support it either!

```scss
iframe {
  height: 100%;
  height: calc(100% - 48px); /* 48px = banner height */
}
```
