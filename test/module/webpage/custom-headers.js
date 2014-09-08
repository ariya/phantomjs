var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();
assert.typeOf(page.customHeaders, 'object');
assert.strictEqual(JSON.stringify(page.customHeaders), '{}');

// NOTE: HTTP header names are case-insensitive. Our test server
// returns the name in lowercase.

page.customHeaders =  {
    'Custom-Key': 'Custom-Value',
    'User-Agent': 'Overriden-UA',
    'Referer': 'Overriden-Referer'
};
page.open('http://localhost:9180/echo', function (status) {
    var json, headers;
    assert.equal(status, 'success');
    json = JSON.parse(page.plainText);
    assert.typeOf(json, 'object');
    headers = json.headers;
    assert.typeOf(headers, 'object');

    assert.isTrue(headers.hasOwnProperty('custom-key'));
    assert.isTrue(headers.hasOwnProperty('user-agent'));
    assert.isTrue(headers.hasOwnProperty('referer'));
    assert.equal(headers['custom-key'], 'Custom-Value');
    assert.equal(headers['user-agent'], 'Overriden-UA');
    assert.equal(headers['referer'], 'Overriden-Referer');
});
