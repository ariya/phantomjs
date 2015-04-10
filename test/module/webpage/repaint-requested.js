var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

var requestCount = 0;

page.onRepaintRequested = function(x, y, w, h) {
    if ((w > 0) && (h > 0)) {
        ++requestCount;
    }
};

page.open('http://localhost:9180/hello.html', function (status) {
    assert.isTrue(requestCount > 0);
});
