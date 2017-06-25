---
layout: post
title:  os
section: system
kind: property
permalink: api/system/property/os.html
---

Read-only. An object providing information about the operating system, including `architecture`, `name`, and `version`. For example:

## Examples

```javascript
var system = require('system');
var os = system.os;
console.log(os.architecture);  // '32bit'
console.log(os.name);  // 'windows'
console.log(os.version);  // '7'
```








