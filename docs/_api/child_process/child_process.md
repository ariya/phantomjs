---
layout: post
title: Child Process Module
section: child_process
permalink: api/child_process/index.html
---

The child_process module allows you to invoke subprocesses and communicate with them via stdin / stdout / stderr. This is useful for tasks such as printing, sending mail, or invoking scripts or programs written in another language (not Javascript).

To start using, you must `require` a reference to the `child_process` module:

```javascript
var process = require("child_process")
var spawn = process.spawn
var execFile = process.execFile

var child = spawn("ls", ["-lF", "/rooot"])

child.stdout.on("data", function (data) {
  console.log("spawnSTDOUT:", JSON.stringify(data))
})

child.stderr.on("data", function (data) {
  console.log("spawnSTDERR:", JSON.stringify(data))
})

child.on("exit", function (code) {
  console.log("spawnEXIT:", code)
})

//child.kill("SIGKILL")

execFile("ls", ["-lF", "/usr"], null, function (err, stdout, stderr) {
  console.log("execFileSTDOUT:", JSON.stringify(stdout))
  console.log("execFileSTDERR:", JSON.stringify(stderr))
})
```
