---
layout: post
title: Network Monitoring
categories: docs docs-learn
permalink: network-monitoring.html
---

Because PhantomJS permits the inspection of network traffic, it is suitable to build various analysis on the network behavior and performance.

All the resource requests and responses can be sniffed using `onResourceRequested` and `onResourceReceived`. A very simple example to log every request and response is illustrated in the script [netlog.js](https://github.com/ariya/phantomjs/blob/master/examples/netlog.js):

```javascript
var page = require('webpage').create();
page.onResourceRequested = function(request) {
  console.log('Request ' + JSON.stringify(request, undefined, 4));
};
page.onResourceReceived = function(response) {
  console.log('Receive ' + JSON.stringify(response, undefined, 4));
};
page.open(url);
```

By collecting the data and reformatting it, another example, [netsniff.js](https://github.com/ariya/phantomjs/blob/master/examples/netsniff.js), exports the network traffic in [HAR format](http://www.softwareishard.com/blog/har-12-spec). Use [HAR viewer](http://www.softwareishard.com/blog/har-viewer) to visualize the result and get the waterfall diagram.

The following shows an examplary waterfall diagram obtained from BBC website:

![Waterfall Diagram](https://lh6.googleusercontent.com/-xoooH5EB6EE/TgnyJ3r9sRI/AAAAAAAAB98/wYJ_VoWED34/s640/bbc-har.png)

For more advanced network analysis, see projects like [Confess.js](https://github.com/jamesgpearce/confess) and [YSlow](http://yslow.org).

The [integration](http://yslow.org/phantomjs/) of YSlow and PhantomJS is very useful for automated web performance. The report can be in TAP (Test Anything Protocol) and JUnit. Running YSlow with PhantomJS in a continuous integration system such as Jenkins is an easy DIY solution to performance monitoring:

![YSlow and Jenkins](http://i.imgur.com/PTD6j.png)
