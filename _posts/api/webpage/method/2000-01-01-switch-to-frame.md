---
layout: post
title:  switchToFrame
categories: api webpage webpage-method
permalink: api/webpage/method/switch-to-frame.html
---

if the page have othe frame, you can use `switchToFrame(frameName)` or `switchToFrame(framePosition)` to exchange it.Other similar methods are `switchToChildFrame()`,`switchToFocusedFrame`,`switchToMainFrame` and `switchToParentFrame`

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();
page.open('http://www.sample.com',function(status){
  if(status!== 'success'){
    console.log('Unable to access network');
  }else{
    page.switchToFrame('framwName/framwPosition');
    console.log(page.frameContent);
    phantom.exit();
	}
});
```








