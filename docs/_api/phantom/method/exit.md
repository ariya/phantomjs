---
layout: post
title:  exit
section: phantom
kind: method
permalink: api/phantom/method/exit.html
---

`phantom.exit(returnValue)` {void}

Exits the program with the specified return value. If no return value is specified, it is set to `0`.

## Examples

```javascript
if (somethingIsWrong) {
  phantom.exit(1);
} else {
  phantom.exit(0);
}
```








