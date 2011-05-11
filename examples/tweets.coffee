if phantom.state.length == 0
    phantom.state = 'tweets'
    phantom.open 'http://mobile.twitter.com/sencha'
else
    list = document.querySelectorAll 'span.status'
    console.log i.innerHTML.replace(/<.*?>/g, '') for i in list
    phantom.exit()
