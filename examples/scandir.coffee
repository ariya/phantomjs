# List all the files in a Tree of Directories

if phantom.args.length != 1
  console.log "Usage: phantomjs scandir.js DIRECTORY_TO_SCAN"
  phantom.exit()
scanDirectory = (path) ->
  if phantom.fs.exists(path) and phantom.fs.isFile(path)
    console.log path
  else if phantom.fs.isDir(path)
    phantom.fs.list(path).forEach (e) ->
      scanDirectory path + "/" + e  if e != "." and e != ".."

scanDirectory phantom.args[0]
phantom.exit()