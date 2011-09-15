var page = require('webpage').create(),
    address = phantom.args[0];

if (phantom.args.length === 0) {
    console.log('Usage: netlog.js <some URL>');
    phantom.exit();
} else {

    page.onResourceRequested = function (req) {
        console.log('requested: ' + JSON.stringify(req, undefined, 4));
    };

    page.onResourceReceived = function (res) {
        console.log('received: ' + JSON.stringify(res, undefined, 4));
    };

    page.open(address, function (status) {
        if (status !== 'success') {
            console.log('FAIL to load the address');
        }
        phantom.exit();
    });
}
