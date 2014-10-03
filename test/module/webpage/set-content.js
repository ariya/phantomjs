var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();
var expectedContent = '<html><body><div>Test div</div></body></html>';
var expectedLocation = 'http://www.phantomjs.org/';
page.setContent(expectedContent, expectedLocation);

var actualContent = page.evaluate(function() {
    return document.documentElement.textContent;
});
assert.equal(actualContent, 'Test div');

var actualLocation = page.evaluate(function() {
    return window.location.href;
});
assert.equal(actualLocation, expectedLocation);

