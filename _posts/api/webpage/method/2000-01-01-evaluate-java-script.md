---
layout: post
title:  evaluateJavaScript
categories: api webpage webpage-method
permalink: api/webpage/method/evaluate-java-script.html
---

`evaluateJavaScript(str)`

Evaluate a function as a string,
Evaluates the given function string in the context of the web page. It is very familiar with [`evaluate`](http://phantomjs.org/api/webpage/method/evaluate.html).

## Examples

### extract phantomjs.org website logo url

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onConsoleMessage = function(msg) {
  console.log('CONSOLE: ' + msg);
};

page.open('http://phantomjs.org/', function(status) {

  var logoUrl = page.evaluateJavaScript('function(){return document.body.querySelector("img").src;}');
  console.log(logoUrl); // http://phantomjs.org/img/phantomjs-logo.png

  phantom.exit();
  
});
```

**Note:** Javascript code should be an one line string, and involved by `function`.







