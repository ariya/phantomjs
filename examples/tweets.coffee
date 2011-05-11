if phantom.state.length == 0
    phantom.state = 'tweets'
    phantom.open 'http://mobile.twitter.com/sencha'
else
    list = document.querySelectorAll 'span.status'
    for elem, index in list
      console.log((index + 1) + ': ' + elem.innerHTML.replace(/<.*?>/g, ''))
    phantom.exit()
