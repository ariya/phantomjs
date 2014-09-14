var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

var url = "http://localhost:9180/cdn-cgi/pe/bag?r%5B%5D=http%3A%2F%2Fwww.example.org%2Fcdn-cgi%2Fnexp%2Fabv%3D927102467%2Fapps%2Fabetterbrowser.js";
var receivedUrl;

page.onResourceRequested = function(requestData, request) {
    request.changeUrl(requestData.url);
};

page.onResourceReceived = function(data) {
    if (data.stage === 'end') {
        receivedUrl = data.url;
    }
};

page.open(url, function (status) {
    assert.equal(status, 'success');
    assert.equal(receivedUrl, url);
});
