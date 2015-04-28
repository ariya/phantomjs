---
layout: post
title:  switchToChildFrame
categories: api webpage webpage-method
permalink: api/webpage/method/switch-to-child-frame.html
---

`switchToChildFrame(frameName)` or `switchToChildFrame(framePosition)`

Deprecated.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();
var url = "http://my.test.com";   // It has iframes;
page.open(url, function(status){
  if ( status == "success" ){
     page.switchToChildFrame(0);  // change context to first iframe
     var result = open.evaluate(function(){
        // javascript code that run in iframes 
     });
     console.log( result );
     page.swtichParentFrame();   // change context to main page
)}


// @TODO: Finish page.switchToChildFrame example.
```








