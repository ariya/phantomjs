var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

var pluginLength = page.evaluate(function() {
    return window.navigator.plugins.length;
});
assert.equal(pluginLength, 0);

page.open('http://localhost:9180/hello.html', function (status) {
    assert.equal(status, 'success');
    var pluginLength = page.evaluate(function() {
        return window.navigator.plugins.length;
    });
    assert.equal(pluginLength, 0);
});
