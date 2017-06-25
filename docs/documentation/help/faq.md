---
layout: post
title: FAQ
categories: docs docs-help
permalink: faq.html
---

**Q: Why there is no binary package or installer for &lt;insert your
favorite OS&gt;?**

A: [Binary packages](download.html), executables, and installer are provided on a volunteer basis.
They may appear later, or they may not exist at all. We are a very small team and we
can't afford to produce all possible combinations of packages for the different
platforms and operating systems out there.

**Q: Why does building PhantomJS take a long time?**

A: Because PhantomJS build workflow bundles the WebKit module, it needs to compile
thousands of source files. Using the binary package is highly recommended.
It is fast to download and [easy to install](download.html).

**Q: Can you give a time estimate of the availability of feature X?**

Because nobody is working full time on PhantomJS, there is no way to
predict the exact timing of the completion of a particular feature.
Every contributor to PhantomJS works on his own pace, sometimes it can
take a few release cycles until a certain feature is shipped.

**Q: Is there any update on issue XYZ??**

Any progress related to an issue, whether it is a change in the plan
or an implementation check-in, will be always posted to the issue
page. This is part of PhantomJS contribution guide, it is essential to
the development workflow. If an issue hasn't received any new update
for a while, a question like "Is there any update?" has an obvious
answer.

**Q: Why do I get build failure with PhantomJS 1.5 after successfully
compiled 1.4?**

A: PhantomJS has a completely different build workflow in version 1.5 compared to
its previous versions (read [the details](http://ariya.ofilabs.com/2012/03/the-evolution-of-phantomjs-build-workflow.html)). If the working directory still has some left-over
from 1.4 build process, there will be a compile error, such as:

```bash
phantom.cpp:305: error: no matching function for call to 'QWebFrame::evaluateJavaScript(QString, QString)
```

The solution is to completely clean the directory from old build files. The
recommended way is to run <tt>git clean -xfd</tt> from the top-level.
**Warning**: this will remove everything not stored in the Git
repository database (read Git documentation for more info). Please back-up any files
appropriately.

After that, run the build script again.

**Q: Why do I get the error message <tt>phantomjs: cannot connect to X
server</tt>?**

A: In PhantomJS 1.4 or earlier, X server is still needed. The workaround is to use Xvfb.
Starting with PhantomJS 1.5, it is **pure headless** and there is no need to
run X11/Xvfb anymore.

**Q: Which WebKit version is used by PhantomJS?**

A: If you want to know HTML5/CSS3/other features supported by PhantomJS, using
WebKit version is not a good idea. See [Supported Web Standards ](http://phantomjs.org/supported-web-standards.html) documentation page for details.

If you really like to get the WebKit version, find it via the user agent, run the
<tt>examples/useragent.js</tt>. The actual version depends on the libraries with
which PhantomJS was compiled.

**Q: Why is PhantomJS not written as Node.js module?**

A: The short answer: "No one can serve two masters."

A longer explanation is as follows.

As of now, it is technically very challenging to do so.

Every Node.js module is essentially "a slave" to the core of Node.js, i.e. "the
master". In its current state, PhantomJS (and its included WebKit) needs to have the
full control (in a synchronous matter) over everything: event loop, network stack,
and JavaScript execution.

If the intention is just about using PhantomJS right from a script running within
Node.js, such a "loose binding" can be achieved by launching a PhantomJS process and
interact with it.

**Q: When using <tt>render()</tt>, why is the background
transparent?**

A: PhantomJS does not set the background color of the web page at all, it is left
to the page to decide its background color. If the page does not set anything, then
it remains transparent.

To set an initial background color for a web page, use the following trick:

```javascript
page.evaluate(function() {
    document.body.bgColor = 'white';
});
```
