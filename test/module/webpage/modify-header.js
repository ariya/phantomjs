var assert = require('../../assert');
var webpage = require('webpage');

// NOTE: HTTP header names are case-insensitive. Our test server
// returns the name in lowercase.

var page = webpage.create();
assert.typeOf(page.customHeaders, 'object');
assert.strictEqual(JSON.stringify(page.customHeaders), '{}');

page.customHeaders = { 'CustomHeader': 'ModifiedCustomValue' };

page.onResourceRequested = function(requestData, request) {
    assert.typeOf(request.setHeader, 'function');
    request.setHeader('CustomHeader', 'ModifiedCustomValue');
};
page.open('http://localhost:9180/echo', function (status) {
    var json, headers;
    assert.equal(status, 'success');
    json = JSON.parse(page.plainText);
    headers = json.headers;
    assert.isTrue(headers.hasOwnProperty('customheader'));
    assert.equal(headers.customheader, 'ModifiedCustomValue');
});

