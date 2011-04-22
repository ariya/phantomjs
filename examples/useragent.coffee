if phantom.state.length is 0
    phantom.state = 'checking'
    phantom.userAgent = 'SpecialAgent'
    phantom.open 'http://www.httpuseragent.org'
else
    console.log document.getElementById('myagent').innerText
    phantom.exit()
