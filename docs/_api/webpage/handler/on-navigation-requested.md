---
layout: post
title:  onNavigationRequested
section: webpage
kind: handler
permalink: api/webpage/handler/on-navigation-requested.html
---

**Introduced:** PhantomJS 1.6

By implementing this callback, you will be notified when a navigation event happens and know if it will be blocked (by `page.navigationLocked`).

### Arguments

* `url`          : The target URL of this navigation event
* `type`         : Possible values include: `'Undefined'`, `'LinkClicked'`, `'FormSubmitted'`, `'BackOrForward'`, `'Reload'`, `'FormResubmitted'`, `'Other'`
* `willNavigate` : `true` if navigation will happen, `false` if it is locked (by `page.navigationLocked`)
* `main`         : `true` if this event comes from the main frame, `false` if it comes from an iframe of some other sub-frame.

## Examples

```javascript
var webPage = require('webpage');
var page = webPage.create();

page.onNavigationRequested = function(url, type, willNavigate, main) {
  console.log('Trying to navigate to: ' + url);
  console.log('Caused by: ' + type);
  console.log('Will actually navigate: ' + willNavigate);
  console.log('Sent from the page\'s main frame: ' + main);
}
```








