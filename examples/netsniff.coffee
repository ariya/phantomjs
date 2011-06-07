page = new WebPage()

if phantom.args.length is 0
    console.log 'Usage: netsniff.js <some URL>'
    phantom.exit()
else
    address = phantom.args[0]

    page.onLoadStarted = ->
        page.startTime = Date.now()

    page.onResourceRequested = (req) ->
        req.time = Date.now() - page.startTime
        resources.push req

    page.open address, (status) ->
        if status isnt 'success'
            console.log 'FAIL to load the address'
        else
            console.log 'All resources:'
            console.log JSON.stringify resources, undefined, 4
        phantom.exit()
