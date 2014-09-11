var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

var urlToBlockRexExp = /logo\.png$/i;
var abortCount = 0;

page.onResourceRequested = function(requestData, request) {
    if (urlToBlockRexExp.test(requestData.url)) {
        assert.typeOf(request, 'object');
        assert.typeOf(request.abort, 'function');
        request.abort();
        ++abortCount;
    }
};

page.open('http://localhost:9180/logo.html', function (status) {
    assert.equal(status, 'success');
    assert.equal(abortCount, 1);
});
