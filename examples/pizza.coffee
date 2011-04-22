# Find pizza in New York using Google Local

if phantom.state.length is 0
    phantom.state = 'pizza'
    phantom.open 'http://www.google.com/m/local?site=local&q=pizza+in+new+york'
else
    list = document.querySelectorAll 'div.bf'
    for item in list
        console.log item.innerText
    phantom.exit()
