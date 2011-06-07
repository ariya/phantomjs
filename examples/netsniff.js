var page = new WebPage(), address, resources = [];

if (phantom.args.length === 0) {
    console.log('Usage: netsniff.js <some URL>');
    phantom.exit();
} else {
    address = phantom.args[0];

    page.onLoadStarted = function () {
        page.startTime = Date.now();
    };

    page.onResourceRequested = function (req) {
        req.time = Date.now() - page.startTime;
        resources.push(req);
    };

    page.open(address, function (status) {
        if (status !== 'success') {
            console.log('FAIL to load the address');
        } else {
            console.log('All resources:');
            console.log(JSON.stringify(resources, undefined, 4));
        }
        phantom.exit();
    });
}
