# echoToFile.coffee - Write in a given file all the parameters passed on the CLI

if phantom.args.length < 2
  console.log "Usage: echoToFile.js DESTINATION_FILE <arguments to echo...>"
  phantom.exit()
else
  content = ""
  i = 1
  while i < phantom.args.length
    content += " " + phantom.args[i]
    ++i
  phantom.saveToFile phantom.args[0], content
  phantom.exit()