# echoToFile.coffee - Write in a given file all the parameters passed on the CLI
fs = require 'fs'

if phantom.args.length < 2
  console.log "Usage: echoToFile.js DESTINATION_FILE <arguments to echo...>"
  phantom.exit()
else
  content = ""
  f = null
  i = 1
  while i < phantom.args.length
    content += phantom.args[i] + (if i == phantom.args.length - 1 then "" else " ")
    ++i
  try
    f = fs.open(phantom.args[0], "w")
    f.writeLine content
  catch e
    console.log e
  phantom.exit()
