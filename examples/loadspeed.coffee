page = require('webpage').create()

if phantom.args.length is 0
  console.log 'Usage: loadspeed.js <some URL>'
  phantom.exit()
else
  t = Date.now()
  address = phantom.args[0]
  page.open address, (status) ->
    if status isnt 'success'
      console.log('FAIL to load the address')
    else
      t = Date.now() - t
      console.log('Page title is ' + page.evaluate( (-> document.title) ))
      console.log('Loading time ' + t + ' msec')
    phantom.exit()

