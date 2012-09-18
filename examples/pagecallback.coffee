p = require("webpage").create()
p.onConsoleMessage = (msg) ->
  console.log msg

p.onCallback = (msg) ->
  console.log "Received by the 'phantom' main context: " + msg
  "Hello there, I'm coming to you from the 'phantom' context instead"

p.evaluate ->
  callbackResponse = callPhantom("Hello, I'm coming to you from the 'page' context")
  console.log "Received by the 'page' context: " + callbackResponse

phantom.exit()
