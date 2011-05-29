page = new WebPage()

page.open 'http://www.httpuseragent.org', (status) ->
  if status isnt 'success'
    console.log 'Unable to access network'
  else
    console.log page.evaluate -> document.getElementById('myagent').innerText
  phantom.exit()
