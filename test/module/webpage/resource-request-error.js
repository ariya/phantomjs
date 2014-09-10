var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

page.onResourceError = function(err) {
    assert.equal(err.status, 404);
    assert.equal(err.statusText, 'File not found');
    assert.equal(err.url, 'http://localhost:9180/notExist.png');
    assert.equal(err.errorCode, 203);
    assert.isTrue(err.errorString.match('Error downloading http://localhost:9180/notExist.png'));
    assert.isTrue(err.errorString.match('server replied: File not found'));
};

page.open('http://localhost:9180/missing-img.html', function (status) {
    assert.equal(status, 'success');
});
