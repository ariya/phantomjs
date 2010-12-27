// Find pizza in New York using Google Local

if (phantom.storage.length === 0) {
    phantom.storage = 'pizza';
    phantom.open('http://www.google.com/m/local?site=local&q=pizza+in+new+york');
} else {
    var list = document.querySelectorAll('div.bf');
    for (var i in list) {
        phantom.log(list[i].innerText);
    }
    phantom.exit();
}
