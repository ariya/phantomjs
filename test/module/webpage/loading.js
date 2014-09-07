var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();
assert.typeOf(page, 'object');
assert.typeOf(page.loading, 'boolean');
assert.typeOf(page.loadingProgress, 'number');

assert.equal(page.loading, false);
assert.equal(page.loadingProgress, 0);
page.open('http://localhost:9180/hello.html');
assert.isTrue(page.loading);
assert.isTrue(page.loadingProgress > 0);

// Another page
page = webpage.create();
assert.equal(page.loading, false);
assert.equal(page.loadingProgress, 0);
page.open('http://localhost:9180/hello.html', function (status) {
    assert.equal(status, 'success');
    assert.equal(page.loading, false);
    assert.equal(page.loadingProgress, 100);
});
