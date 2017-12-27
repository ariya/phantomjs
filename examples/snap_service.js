var page = require('webpage').create();
var server = require('webserver').create();
var system = require('system');
var port;

if (system.args.length !== 2) {
    console.log('Usage: snap_service.js <some port>');
    phantom.exit(1);
} else {
    port = system.args[1];
    var listening = server.listen(port, function(request, response) {

        console.log(JSON.stringify(request, null, 4));

        var url = null,
            format = 'png',
            quality = -1;

        if (request.query) {
            if (request.query.url)
                url = request.query.url;
            if (request.query.format)
                format = request.query.format;
            if (request.query.quality)
                quality = parseFloat(request.query.quality);
        }

        if (request.post) {
            if (request.post.url)
                url = request.post.url;
            if (request.post.format)
                format = request.post.format;
            if (request.post.quality)
                quality = parseFloat(request.post.quality);
        }

        console.log("URL:", url);
        console.log("Format:", format);
        console.log("Quality:", quality);

        if (!url || url.length == 0) {
            response.write("");
            response.close();
        } else {

            var page = require('webpage').create();

            page.viewportSize = {
                width: 800,
                height: 600
            };

            page.open(url, function start(status) {

                // Buffer is an Uint8ClampedArray
                var buffer = page.renderBuffer(format, quality);

                response.statusCode = 200;
                response.headers = {
                    "Cache": "no-cache",
                    "Content-Type": "image/" + format
                };
                page.clearCookies();
                page.close();

                // Pass the Buffer to 'write' to send Uint8ClampedArray
                response.write(buffer);
                response.close();
            });
        }
    });
    if (!listening) {
        console.log("could not create web server listening on port " + port);
        phantom.exit();
    }
}