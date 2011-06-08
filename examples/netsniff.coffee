page = new WebPage()
requests = []
responses = []

if phantom.args.length is 0
    console.log 'Usage: netsniff.js <some URL>'
    phantom.exit()
else
    address = phantom.args[0]

    page.onLoadStarted = ->
        page.startTime = Date.now()

    page.onResourceRequested = (req) ->
        req.time = Date.now() - page.startTime
        requests.push req

    page.onResourceReceived = (res) ->
        res.time = Date.now() - page.startTime
        responses.push res

    page.open address, (status) ->
        if status isnt 'success'
            console.log 'FAIL to load the address'
        else
            console.log 'All requests:'
            console.log JSON.stringify requests, undefined, 4
            console.log ''
            console.log 'All responses:'
            console.log JSON.stringify responses, undefined, 4
        phantom.exit()
