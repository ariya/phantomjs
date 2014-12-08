var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

assert.typeOf(page, 'object');
assert.notEqual(page, null);

assert.equal(page.objectName, 'WebPage');
assert.jsonEqual(page.paperSize, {});

assert.notEqual(page.settings, null);
assert.notEqual(page.settings, {});
