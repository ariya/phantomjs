###
Sort integers from the command line in a very ridiculous way: leveraging timeouts :P
###

if phantom.args < 1
  console.log "Usage: phantomjs sleepsort.js PUT YOUR INTEGERS HERE SEPARATED BY SPACES"
  phantom.exit()
else
  sortedCount = 0
  for int in phantom.args
    setTimeout ((j) ->
      ->
        console.log j
        ++sortedCount
        phantom.exit() if sortedCount is phantom.args.length)(int),
      int

