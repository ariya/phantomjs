var port, server, service;

if (phantom.args.length !== 1) {
    console.log('Usage: simpleserver.js <portnumber>');
    phantom.exit();
} else {
    port = phantom.args[0];
    server = require('webserver').create();

    service = server.listen(port, function (request, response) {

        function excludeObjectName(key, value) {
            // objectName belongs to QObject
            return (key === 'objectName') ? undefined : value;
        }

        console.log('Request at ' + new Date());
        console.log(JSON.stringify(request, excludeObjectName, 4));

        response.statusCode = 200;
        response.headers = {
            'Cache': 'no-cache',
            'Content-Type': 'text/html'
        };
        response.writeBody('<html>');
        response.writeBody('<head>');
        response.writeBody('<title>Hello, world!</title>');
        response.writeBody('</head>');
        response.writeBody('<body>');
        response.writeBody('<p>This is from PhantomJS web server.</p>');
        response.writeBody('<p>Request data:</p>');
        response.writeBody('<pre>');
        response.writeBody(JSON.stringify(request, excludeObjectName, 4));
        response.writeBody('</pre>');
        response.writeBody('</body>');
        response.writeBody('</html>');
    });

    if (service) {
        console.log('Web server running on port ' + port);
    } else {
        console.log('Error: Could not create web server listening on port ' + port);
        phantom.exit();
    }
}
