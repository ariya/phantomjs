page = require("webpage").create()
system = require("system")
host = undefined
port = undefined
address = undefined
if system.args.length < 4
  console.log "Usage: openurlwithproxy.js <proxyHost> <proxyPort> <URL>"
  phantom.exit 1
else
  host = system.args[1]
  port = system.args[2]
  address = system.args[3]
  phantom.setProxy host, port, "manual", "", ""
  page.open address, (status) ->
    if status isnt "success"
      console.log "FAIL to load the address \"" + address + "\" using proxy \"" + host + ":" + port + "\""
    else
      console.log "Page title is " + page.evaluate(->
        document.title
      )
    phantom.exit()
    return
