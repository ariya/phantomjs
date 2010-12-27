if (phantom.storage.length === 0) {
    if (phantom.arguments.length === 0) {
        phantom.log('Usage: loadspeed.js <some URL>');
        phantom.exit();
    } else {
        var address = phantom.arguments[0];
        phantom.storage = Date.now().toString();
        phantom.log('Loading ' + address);
        phantom.open(address);
    }
} else {
    var elapsed = Date.now() - new Date().setTime(phantom.storage);
    if (phantom.loadStatus === 'success') {
        phantom.log('Page title is ' + document.title);
        phantom.log('Loading time ' + elapsed + ' msec');
    } else {
        phantom.log('FAIL to load the address');
    }
    phantom.exit();
}
