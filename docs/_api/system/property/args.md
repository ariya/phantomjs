---
layout: post
title:  args
section: system
kind: property
permalink: api/system/property/args.html
---

`system.args` {String[]}

Queries and returns a list of the command-line arguments.  The first one is always the script name, which is then followed by the subsequent arguments.

## Examples

The following example prints all of the command-line arguments:

```javascript
var system = require('system');
var args = system.args;

if (args.length === 1) {
  console.log('Try to pass some arguments when invoking this script!');
} else {
  args.forEach(function(arg, i) {
    console.log(i + ': ' + arg);
  });
}
```








