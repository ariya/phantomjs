---
layout: post
title:  injectJs
section: webpage
kind: method
permalink: api/webpage/method/inject-js.html
---

`injectJs(filename)` {boolean}

Injects external script code from the specified file into the page (like `page.includeJs`, except that the file does not need to be accessible from the hosted page).

If the file cannot be found in the current directory, `libraryPath` is used for additional look up.

This function returns `true` if injection is successful, otherwise it returns `false`.

## Examples

### Inject do.js file into phantomjs.org page

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.open('http://www.phantomjs.org', function(status) {
  if (status === "success") {
    page.includeJs('http://ajax.googleapis.com/ajax/libs/jquery/1.7.2/jquery.min.js', function() {
      if (page.injectJs('do.js')) {
        var title = page.evaluate(function() {
          // returnTitle is a function loaded from our do.js file - see below
          return returnTitle();
        });
        console.log(title);
        phantom.exit();
      }
    });
  }
});
```

Where do.js is simply:
```javascript
window.returnTitle = function() {
  return document.title;
};
```

The console log will be:
```javascript
"PhantomJS | PhantomJS"
```


**Note:** The arguments and the return value to the `evaluate` function must be a simple primitive object. The rule of thumb: if it can be serialized via JSON, then it is fine.

**Closures, functions, DOM nodes, etc. will _not_ work!**

