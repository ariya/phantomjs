---
layout: post
title:  evaluateJavaScript
categories: api webpage webpage-method
permalink: api/webpage/method/evaluate-java-script.html
---

`evaluateJavaScript(str)`

Evaluate a function contained in a string. 

`evaluateJavaScript` evaluates the function defined in the string in the context of the web page. It is similar to [`evaluate`](http://phantomjs.org/api/webpage/method/evaluate.html).

## Examples

### Set a variable and log it from the web page

This example passes a constant value from phantomjs to the `window` object in the context of the web page, and then logs that value. 

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onConsoleMessage = function(msg) {
  console.log('The web page said: ' + msg);
};

page.open('http://phantomjs.org/', function(status) {
  var script1 = "function(){ window.phantomVar='phantomjs made me do it!'; }";
  var script2 = "function(){ console.log(window.phantomVar); }";
  page.evaluateJavaScript(script1);
  page.evaluateJavaScript(script2);
  phantom.exit();
});
```

Notice that `str` must contain the text of a function declaration. The declared function is invoked immediately.  

If you try to use it simply to define a variable like this. it won't work.

```javascript
  page.evaluateJavaScript("window.phantomVar='phantomjs made me do it!';"); /*wrong*/
```
If you try this you'll get an error message like this:

    SyntaxError: Expected token ')'
        phantomjs://webpage.evaluate():1 in evaluateJavaScript

### Extract the phantomjs.org website's logo url

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
