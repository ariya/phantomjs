// Find pizza in New York using Google Local

if (phantom.state.length === 0) {
    phantom.state = 'pizza';
    phantom.open('http://www.google.com/m/local?site=local&q=pizza+in+new+york');
} else {
    var list = document.querySelectorAll('div.bf');
    for (var i = 0; i < list.length; i++) {
        console.log(list[i].innerText);
    }
    phantom.exit();
}
