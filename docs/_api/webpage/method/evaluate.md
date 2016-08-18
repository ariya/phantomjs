---
layout: post
title:  evaluate
section: webpage
kind: method
permalink: api/webpage/method/evaluate.html
---

`evaluate(function, arg1, arg2, ...)` {object}

Evaluates the given function in the context of the web page. The execution is sandboxed, the web page has no access to the `phantom` object and it can't probe its own setting.

## Examples

### Get the page title from Bing.com (1)

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.open('http://m.bing.com', function(status) {

  var title = page.evaluate(function() {
    return document.title;
  });

  console.log(title);
  phantom.exit();

});
```

### Get the page title from Bing.com (2)

As of PhantomJS 1.6, JSON-serializable arguments can be passed to the function. In the following example, the text value of a DOM element is extracted.

The following example achieves the same end goal as the previous example but the element is chosen based on a selector which is passed to the `evaluate` call:

```javascript
page.open('http://m.bing.com', function(status) {

  var title = page.evaluate(function(s) {
    return document.querySelector(s).innerText;
  }, 'title');

  console.log(title);
  phantom.exit();

});
```

**Note:** The arguments and the return value to the `evaluate` function must be a simple primitive object. The rule of thumb: if it can be serialized via JSON, then it is fine.

**Closures, functions, DOM nodes, etc. will _not_ work!**

Any console message from a web page, including from the code inside `evaluate`, will not be displayed by default. To override this behavior, use the `onConsoleMessage` callback. The first example can be rewritten to:

### Get the page title from Bing.com and print it inside `evaluate`

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onConsoleMessage = function(msg) {
  console.log(msg);
}

page.open('http://m.bing.com', function(status) {

  page.evaluate(function() {
    console.log(document.title);
  });

  phantom.exit();

});
```





