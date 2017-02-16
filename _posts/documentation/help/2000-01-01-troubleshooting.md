---
layout: post
title: Troubleshooting
categories: docs docs-help
permalink: troubleshooting.html
---

### Latest Version

Before reporting an issue, make sure you are using the latest version:

```
phantomjs --version
```

All examples are designed to work with the latest version of PhantomJS. If some examples do not work, make sure that there is not more than one version of PhantomJS installed in the system. Multiple versions may lead to conflict as to which one is being invoked in the terminal.

### Network Problem

If the data is not transferred correctly, check if the network works as expected.

Since every network request and response can be "sniffed", add simple callbacks to facilitate such troubleshooting. For more details, see the wiki page on [Network Monitoring](network-monitoring.html). An example of a simple logging:

```javascript
page.onResourceRequested = function (request) {
    console.log('Request ' + JSON.stringify(request, undefined, 4));
};
```

Transport Layer Security (TLS) and Secure Sockets Layer (SSL) are necessary to access encrypted data, for example when connecting to a server using HTTPS. Thus, if PhantomJS works well with HTTP but it shows some problem when using HTTPS, the first useful thing to check it whether the SSL libraries, usually OpenSSL, have been installed properly.

On Windows, the default proxy setting may cause a massive network latency (see Known Issues in the release note). The workaround is to disable proxy completely, e.g. by launching PhantomJS with `--proxy-type=none` command-line argument.

### SELinux

SELinux can stop PhantomJS working. The solution given here, to create a custom policy, has been reported to work: http://serverfault.com/a/430499/87322


### Error Handling

To easily catch an error occured in a web page, whether it is a syntax error or other thrown exception, an `onError` handler for the WebPage object has been added. An example on such a handler is:

```javascript
page.onError = function (msg, trace) {
    console.log(msg);
    trace.forEach(function(item) {
        console.log('  ', item.file, ':', item.line);
    });
};
```

Now if the page opens a site with some JavaScript exceptions, a detailed information (including the stack trace) will be printed out.

### Remote Debugging

Remote debugging permits inspection of the script and web page via another WebKit-based browser (Safari and Chrome). This is achieved by launching PhantomJS with the new option, as in this example

```
phantomjs --remote-debugger-port=9000 test.js
```

After that, open Safari/Chrome/Chromium and go to the URL <tt>http://ipaddress:9000</tt>.  If you executed the <code>phantomjs</code> command on the same machine, it will be <tt>http://127.0.0.1:9000</tt>

You will see a list of links.  Click the first (it may say "about:blank").  It will show a version of the [Web Inspector interface](http://www.webkit.org/blog/1620/webkit-remote-debugging/) which in this case works on the script being tested..

To set breakpoints, look for your script in the drop-down in the Scripts tab; it should have the same URL that you just clicked on.

After setting breakpoints (or if you are using <code>debugger</code> instead), to run your script, simply enter the ```__run()``` command in the Web Inspector Console. Alternatively, use `--remote-debugger-autorun=yes` command-line argument to have your script launched immediately.

To debug inside a target web page requires two inspectors and a multi-step process using code like this:

```javascript
// ... do some stuff that gives you access to a "page" instance ...
console.log("Refresh a second debugger-port page and open a second webkit inspector for the target page.");
console.log("Letting this page continue will then trigger a break in the target page.");
debugger; // pause here in first web browser tab on step 5
page.evaluateAsync(function() {
    debugger; // step 9 will wait here in the second web browser tab
});
```
1. Instrument your PhantomJS script with two `debugger;` lines as in the example above.
1. Start PhantomJS on the command line with the `--remote-debugger-port=9000` option.
1. Open http://127.0.0.1:9000/ in a Webkit-based web browser.
1. There should be a bulleted list with a single link. Click on it to open a web inspector that operates in the *PhantomJS* context. (Sometimes the link is invisible because it contains no text; if you see nothing, navigate directly to http://127.0.0.1:9000//webkit/inspector/inspector.html?page=1.)
1. Change to the Console section of the inspector and execute the statement `__run()`. This should cause the script to run until your first `debugger;` statement.
1. Open a new browser tab and return to the debugging portal at http://127.0.0.1:9000. There should now be a second entry in the bulleted list.
1. Click on this entry to open a new debugger that operates in the context of *your page* inside PhantomJS.
1. Return to the first web inspector tab and click the "Continue" button in the debugger.
1. In the second tab, you should now find the JS debugger paused at the second `debugger;` statement. You can now debug your page's JS using the full capabilities of the Webkit inspector.


### Interactive Mode (REPL)

If PhantomJS is launched without any argument, it starts in the so-called interactive mode, also known as [REPL](https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop) (read-eval-print-loop). This mode allows a faster cycle of experiment and script prototyping. PhantomJS REPL supports the expected features: command editing, persistent history, and autocomplete (with Tab key). Read more [about REPL](http://phantomjs.org/repl.html).
