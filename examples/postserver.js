// Example using HTTP POST operation

var page = require('webpage').create(),
    server = require('webserver').create(),
    port = 12345,
    data = 'universe=expanding&answer=42';

server.listen(port, function(request, response) {
  response.statusCode = 200;
  response.headers = {
      'Cache': 'no-cache',
      'Content-Type': 'text/plain;charset=utf-8'
  };
  response.write(JSON.stringify(request));
});

page.open("http://localhost:" + port + "/", 'post', data, function (status) {
    if (status !== 'success') {
        console.log('Unable to post!');
    } else {
        console.log(page.content);
    }
    phantom.exit();
});
