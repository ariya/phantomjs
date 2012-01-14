var page = require('webpage').create();
var server = require('webserver').create();
var system = require('system');
var host, port;

if (system.args.length !== 2) {
    console.log('Usage: server.js <some port>');
    phantom.exit();
} else {
    port = system.args[1];
    var listening = server.listen(port, function (request, response) {
        console.log("GOT HTTP REQUEST");
        console.log("request.url = " + request.url);
        console.log("request.queryString = " + request.queryString);
        console.log("request.method = " + request.method);
        console.log("request.httpVersion = " + request.httpVersion);
        console.log("request.statusCode = " + request.statusCode);
        console.log("request.isSSL = " + request.isSSL);
        console.log("request.remoteIP = " + request.remoteIP);
        console.log("request.remotePort = " + request.remotePort);
        console.log("request.remoteUser = " + request.remoteUser);
        console.log("request.headers = " + request.headers);
        for(var i = 0; i < request.headers; ++i) {
            console.log("request.headerName(" + i + ") = " + request.headerName(i));
            console.log("request.headerValue(" + i + ") = " + request.headerValue(i));
        }

        // we set the headers here
        response.statusCode = 200;
        response.headers = {"Cache": "no-cache", "Content-Type": "text/html"};
        // this is also possible:
        response.setHeader("foo", "bar");
        // now we write the body
        // note: the headers above will now be sent implictly
        response.write("<html><head><title>YES!</title></head>");
        // note: writeBody can be called multiple times
        response.write("<body><p>pretty cool :)</body></html>");
    });
    if (!listening) {
        console.log("could not create web server listening on port " + port);
        phantom.exit();
    }
    var url = "http://localhost:" + port + "/foo/bar.php?asdf=true";
    console.log(url);
    page.open(url, function (status) {
        if (status !== 'success') {
            console.log('FAIL to load the address');
        } else {
            console.log(page.content);
        }
        phantom.exit();
    });
}
