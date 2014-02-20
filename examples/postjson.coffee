# Example using HTTP POST operation

page = require("webpage").create()
server = "http://posttestserver.com/post.php?dump"
data = "{\"universe\": \"expanding\", \"answer\": 42}"
headers = "Content-Type": "application/json"

page.open server, "post", data, headers, (status) ->
  if status isnt "success"
    console.log "Unable to post!"
  else
    console.log page.content
  phantom.exit()
