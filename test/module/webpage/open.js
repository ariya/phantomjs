var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();
assert.typeOf(page, 'object');

page.open('http://localhost:9180/hello.html', function (status) {
    assert.equal(status, 'success');
    assert.typeOf(page.title, 'string');
    assert.equal(page.title, 'Hello');
    assert.typeOf(page.plainText, 'string');
    assert.equal(page.plainText, 'Hello, world!');
});
