if phantom.state.length is 0
    address = phantom.args[0]
    phantom.state = 'news'
    phantom.viewportSize = { width: 320, height: 480 }
    phantom.open 'http://news.google.com/news/i/section?&topic=t'
else
    body = document.body
    body.style.backgroundColor = '#fff'
    body.querySelector('div#title-block').style.display = 'none'
    body.querySelector('form#edition-picker-form').parentElement.parentElement.style.display = 'none'
    phantom.sleep 500
    phantom.render 'technews.png'
    phantom.exit()
