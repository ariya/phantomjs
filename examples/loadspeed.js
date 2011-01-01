if (phantom.storage.length === 0) {
    if (phantom.arguments.length === 0) {
        console.log('Usage: loadspeed.js <some URL>');
        phantom.exit();
    } else {
        var address = phantom.arguments[0];
        phantom.storage = Date.now().toString();
        console.log('Loading ' + address);
        phantom.open(address);
    }
} else {
    var elapsed = Date.now() - new Date().setTime(phantom.storage);
    if (phantom.loadStatus === 'success') {
        console.log('Page title is ' + document.title);
        console.log('Loading time ' + elapsed + ' msec');
    } else {
        console.log('FAIL to load the address');
    }
    phantom.exit();
}
