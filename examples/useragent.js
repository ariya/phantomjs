if (phantom.storage.length === 0) {
    phantom.storage = 'checking';
    phantom.userAgent = 'SpecialAgent';
    phantom.open('http://www.httpuseragent.org');
} else {
    phantom.log(document.getElementById('myagent').innerText);
    phantom.exit();
}
