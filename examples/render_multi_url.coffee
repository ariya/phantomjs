# Render Multiple URLs to file
# FIXME: For now it is fine with pure domain names: don't think it would work with paths and stuff like that

# Extend the Array Prototype with a 'foreach'
Array.prototype.forEach = (action) ->
    for i, j in this
        action j, i, _len

# Render a given url to a given file
# @param url URL to render
# @param file File to render to
# @param callback Callback function
renderUrlToFile = (url, file, callback) ->
    page = require('webpage').create()
    page.viewportSize = { width: 800, height : 600 }
    page.settings.userAgent = 'Phantom.js bot'

    page.open url, (status) ->
       if status isnt 'success'
           console.log "Unable to render '#{url}'"
       else
           page.render file

       delete page
       callback url, file

# Read the passed args
if phantom.args.length > 0
    arrayOfUrls = phantom.args
else
    # Default (no args passed)
    console.log 'Usage: phantomjs render_multi_url.coffee [domain.name1, domain.name2, ...]'
    arrayOfUrls = [
      'www.google.com',
      'www.bbc.co.uk',
      'www.phantomjs.org'
    ]

# For each URL
arrayOfUrls.forEach (pos, url, total) ->
    file_name = "./#{url}.png"

    # Render to a file
    renderUrlToFile "http://#{url}", file_name, (url, file) ->
        console.log "Rendered '#{url}' at '#{file}'"
        if pos is total - 1
            # Close Phantom if it's the last URL
            phantom.exit()
