---
layout: post
title: Child Process Module
categories: api child_process
permalink: api/child_process/index.html
---

The child_process module allows you to invoke subprocesses and communicate with them via stdin / stdout / stderr. This is useful for tasks such as printing, sending mail, or invoking scripts or programs written in another language (not Javascript).

To start using, you must `require` a reference to the `child_process` module:

```javascript
var childProcess = require('child_process');
```
