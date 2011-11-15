var page = require('webpage').create();
var server = require('webserver').create();
var host, port;

if (phantom.args.length !== 1) {
    console.log('Usage: server.js <some port>');
    phantom.exit();
} else {
    port = phantom.args[0];
    server.listen(port, function () {
        console.log("GOT HTTP REQUEST");
    });
    console.log("http://localhost:" + port + "/");
    page.open("http://localhost:" + port + "/", function (status) {
        if (status !== 'success') {
            console.log('FAIL to load the address');
        } else {
            console.log('Page title is ' + page.evaluate(function () {
                return document.title;
            }));
        }
        phantom.exit();
    });
}
