---
layout: post
title:  pid
section: system
kind: property
permalink: api/system/property/pid.html
---

`system.pid` {Number}

**Introduced:** PhantomJS 1.8
Read-only. The PID (Process ID) for the currently executing PhantomJS process.

## Examples

```javascript
var system = require('system');
var pid = system.pid;

console.log(pid);
```








