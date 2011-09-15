page = require('webpage').create()
address = phantom.args[0]

if phantom.args.length is 0
    console.log 'Usage: netlog.coffee <some URL>'
    phantom.exit()
else
    page.onResourceRequested = (req) ->
        console.log 'requested ' + JSON.stringify(req, undefined, 4)

    page.onResourceReceived = (res) ->
        console.log 'received ' + JSON.stringify(res, undefined, 4)

    page.open address, (status) ->
        if status isnt 'success'
            console.log 'FAIL to load the address'
        phantom.exit()
