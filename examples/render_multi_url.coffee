# Render Multiple URLs to file
# FIXME: For now it is fine with pure domain names: don't think it would work
# with paths and stuff like that

system = require 'system'
worker = 0

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
      page.close()
      callback url, file
    else
      window.setTimeout ->
        page.render file
        page.close()
        callback url, file
      , 200

# Read the passed args
if system.args.length > 1
  arrayOfUrls = system.args[1..]
else
  # Default (no args passed)
  console.log 'Usage: phantomjs render_multi_url.coffee [domain.name1, domain.name2, ...]'
  arrayOfUrls = [
    'www.google.com',
    'www.bbc.co.uk',
    'www.phantomjs.org'
  ]

worker += arrayOfUrls.length

# For each URL
for url in arrayOfUrls
  file_name = "./#{url}.png"

  # Render to a file
  renderUrlToFile "http://#{url}", file_name, (url, file) ->
    console.log "Rendered '#{url}' at '#{file}'"
    worker--
    if worker is 0
      # Close Phantom if it's the last URL
      phantom.exit()
