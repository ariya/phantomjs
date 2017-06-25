---
layout: post
title:  onError
section: phantom
kind: handler
permalink: api/phantom/handler/on-error.html
---

**Introduced:** PhantomJS 1.5

This callback is invoked when there is a JavaScript execution error _not_ caught by a `page.onError` handler. This is the closest it gets to having a global error handler in PhantomJS, and so it is a best practice to set this `onError` handler up in order to catch any unexpected problems. The arguments passed to the callback are the error message and the stack trace [as an Array].

## Examples

```javascript
phantom.onError = function(msg, trace) {
  var msgStack = ['PHANTOM ERROR: ' + msg];
  if (trace && trace.length) {
    msgStack.push('TRACE:');
    trace.forEach(function(t) {
      msgStack.push(' -> ' + (t.file || t.sourceURL) + ': ' + t.line + (t.function ? ' (in function ' + t.function +')' : ''));
    });
  }
  console.error(msgStack.join('\n'));
  phantom.exit(1);
};
```








