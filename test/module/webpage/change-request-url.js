var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

var urlToChange = 'http://localhost:9180/logo.png';
var alternativeUrl = 'http://localhost:9180/phantomjs-logo.gif';
var startStage = 0;
var endStage = 0;

page.onResourceRequested = function(requestData, request) {
    if (requestData.url === urlToChange) {
        assert.typeOf(request, 'object');
        assert.typeOf(request.changeUrl, 'function');
        request.changeUrl(alternativeUrl);
    }
};

page.onResourceReceived = function(data) {
    if (data.url === alternativeUrl && data.stage === 'start') {
        ++startStage;
    }
    if (data.url === alternativeUrl && data.stage === 'end') {
        ++endStage;
    }
};

page.open('http://localhost:9180/logo.html', function (status) {
    assert.equal(status, 'success');
    assert.equal(startStage, 1);
    assert.equal(endStage, 1);
    assert.equal(page.content.match('logo.png'), 'logo.png');
});
