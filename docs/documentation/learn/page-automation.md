---
layout: post
title: Page Automation
categories: docs docs-learn
permalink: page-automation.html
---

Because PhantomJS can load and manipulate a web page, it is perfect to carry out various page automations.

## DOM Manipulation

Since the script is executed as if it is running on a web browser, standard [DOM scripting](http://en.wikipedia.org/wiki/DOM_scripting) and [CSS selectors](http://www.w3.org/TR/css3-selectors/) work just fine.

The following `useragent.js` example demonstrates reading the `textContent` property of the element whose *id* is `myagent`:

```javascript
var page = require('webpage').create();
console.log('The default user agent is ' + page.settings.userAgent);
page.settings.userAgent = 'SpecialAgent';
page.open('http://www.httpuseragent.org', function(status) {
  if (status !== 'success') {
    console.log('Unable to access network');
  } else {
    var ua = page.evaluate(function() {
      return document.getElementById('myagent').textContent;
    });
    console.log(ua);
  }
  phantom.exit();
});
```
The above example also demonstrates a way to customize the user agent seen by the remote web server.

## Use jQuery and Other Libraries

As of version 1.6, you are also able to include jQuery into your page using a page.includeJs as follows:

```javascript
var page = require('webpage').create();
page.open('http://www.sample.com', function() {
  page.includeJs("http://ajax.googleapis.com/ajax/libs/jquery/1.6.1/jquery.min.js", function() {
    page.evaluate(function() {
      $("button").click();
    });
    phantom.exit()
  });
});
```

The above snippet will open up a web page, include the jQuery library into the page, and then click on all buttons using jQuery. It will then exit from the web page. Make sure to put the exit statement within the page.includeJs or else it may exit prematurely before the javascript code is included.

## The Webpage instance

Suppose you have an instance of the webpage:

```javascript
var page = require('webpage').create();
```
What can be extracted and executed on it?

#### Attributes
* page.canGoForward -> boolean

If window.history.forward would be a valid action

* page.canGoBack -> boolean

If window.history.back would be a valid action

* page.clipRect -> object

Can be set to an object of the following form:

```javascript
{ top: 0, left: 0, width: 1024, height: 768 }
```
It specifies which part of the screen will be taken in the screenshot

* page.content -> string

The whole HTML content of the page

* page.cookies -> object

The cookies.
They have this form:

```javascript
{
    'name' : 'Valid-Cookie-Name',
    'value' : 'Valid-Cookie-Value',
    'domain' : 'localhost',
    'path' : '/foo',
    'httponly' : true,
    'secure' : false
}
```

* page.customHeaders -> object

TODO

* page.event -> object 

Contains modifiers and keys
TODO

* page.libraryPath -> string 

The current library path, usually it's the directory where the script
is executed from

* page.loading -> boolean 

If the page is loading or not

* page.loadingProgress -> number 

The percentage that has been loaded. 100 means that the page is loaded.

* page.navigationLocked -> boolean 

TODO

* page.offlineStoragePath -> string 
Where the sqlite3 localstorage and other offline data are stored.

* page.offlineStorageQuota, 'number 

The quota in bytes that can be stored offline

* page.paperSize -> object 

Similar to clipRect but takes real paper sizes such as A4.
For an in depth example check  [this](https://github.com/ariya/phantomjs/blob/d10b8dc5832797be434f43fa2cbd4f1110d035fb/examples/printheaderfooter.js).

* page.plainText -> string 

The elements that are plain text in the page

* page.scrollPosition -> object 

The current scrolling position as an object of the following form:

```javascript
{
	left: 0
	top: 0
}
```

* page.settings -> object 

The settings which currently only has the useragent string
page.settings.userAgent = 'SpecialAgent';

* page.title -> string 

The page title

* page.url -> string 

The page url

* page.viewportSize -> object 

The browser size which is in the following form:

```javascript
{
	width: 1024,
	height: 768
}
```

* page.windowName -> string 

The name of the browser window that is assigned by the WM.

* page.zoomFactor -> number 

The zoom factor. 1 is the normal zoom.


#### Functions
* page.childFramesCount
* page.childFramesName
* page.close
* page.currentFrameName
* page.deleteLater
* page.destroyed
* page.evaluate
* page.initialized
* page.injectJs
* page.javaScriptAlertSent
* page.javaScriptConsoleMessageSent
* page.loadFinished
* page.loadStarted
* page.openUrl
* page.release
* page.render
* page.resourceError
* page.resourceReceived
* page.resourceRequested
* page.uploadFile
* page.sendEvent
* page.setContent
* page.switchToChildFrame
* page.switchToMainFrame
* page.switchToParentFrame
* page.addCookie
* page.deleteCookie
* page.clearCookies

#### Handlers/Callbacks
List of all the page events:

* onInitialized
* onLoadStarted
* onLoadFinished
* onUrlChanged
* onNavigationRequested
* onRepaintRequested
* onResourceRequested
* onResourceReceived
* onResourceError
* onResourceTimeout
* onAlert
* onConsoleMessage
* onClosing

For more information check this in depth [example](https://github.com/ariya/phantomjs/blob/master/examples/page_events.js).
