var page = new WebPage(), address, requests = [], responses = [];

if (phantom.args.length === 0) {
    console.log('Usage: netsniff.js <some URL>');
    phantom.exit();
} else {
    address = phantom.args[0];

    page.onLoadStarted = function () {
        page.startTime = Date.now();
    };

    page.onResourceRequested = function (req) {
        requests.push(req);
    };

    page.onResourceReceived = function (res) {
        responses.push(res);
    };

    page.open(address, function (status) {
        if (status !== 'success') {
            console.log('FAIL to load the address');
        } else {
            console.log('All requests:');
            console.log(JSON.stringify(requests, undefined, 4));
            console.log();
            console.log('All responses:');
            console.log(JSON.stringify(responses, undefined, 4));
        }
        phantom.exit();
    });
}
