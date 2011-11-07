var page = require('webpage').create();
var server = require('webserver').create();
var host, port;

if (phantom.args.length !== 1) {
    console.log('Usage: server.js <some port>');
    phantom.exit();
} else {
    port = phantom.args[0];
    server.listen(port, function (request, response) {
        console.log("GOT HTTP REQUEST");
        console.log("request.url() = " + request.url());
        console.log("request.queryString() = " + request.queryString());
        console.log("request.method() = " + request.method());
        console.log("request.httpVersion() = " + request.httpVersion());
        console.log("request.statusCode() = " + request.statusCode());
        console.log("request.isSSL() = " + request.isSSL());
        console.log("request.remoteIP() = " + request.remoteIP());
        console.log("request.remotePort() = " + request.remotePort());
        console.log("request.remoteUser() = " + request.remoteUser());

        var headers = "HTTP/1.1 200 OK\r\n" +
                      "Cache: no-cache\r\n" +
                      "Content-Type: text/html\r\n" +
                      "\r\n";
        response.writeHeaders(headers);
        var contents = "<html><head><title>YES!</title></head>" +
                       "<body><p>pretty cool :)</body></html>";
        response.writeBody(contents);
    });
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