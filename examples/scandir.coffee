# List all the files in a Tree of Directories

if phantom.args.length != 1
  console.log "Usage: phantomjs scandir.js DIRECTORY_TO_SCAN"
  phantom.exit()
scanDirectory = (path) ->
  fs = require 'fs'
  if fs.exists(path) and fs.isFile(path)
    console.log path
  else if fs.isDirectory(path)
    fs.list(path).forEach (e) ->
      scanDirectory path + "/" + e  if e != "." and e != ".."

scanDirectory phantom.args[0]
phantom.exit()
