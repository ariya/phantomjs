var page = require('webpage').create();
var server = require('webserver').create();
var host, port;

if (phantom.args.length !== 1) {
    console.log('Usage: server.js <some port>');
    phantom.exit();
} else {
    port = phantom.args[0];
    server.listen(port, function (request, response) {
        console.log("GOT HTTP REQUEST: url=" + request.url() + ", method: " + request.method());
        var headers = "HTTP/1.1 200 OK\r\n" +
                      "Cache: no-cache\r\n" +
                      "Content-Type: text/html\r\n" +
                      "\r\n";
        response.writeHeaders(headers);
        var contents = "<html><head><title>YES!</title></head>" +
                       "<body><p>pretty cool :)</body></html>";
        response.writeBody(contents);
    });
    console.log("http://localhost:" + port + "/");
    page.open("http://localhost:" + port + "/", function (status) {
        if (status !== 'success') {
            console.log('FAIL to load the address');
        } else {
            console.log(page.content);
        }
        phantom.exit();
    });
}
