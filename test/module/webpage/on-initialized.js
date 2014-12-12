var assert = require('../../assert');
var page = require('webpage').create();

assert.equal(page.onInitialized, undefined);

var onInitialized1 = function() { var x = "x"; };
page.onInitialized = onInitialized1;
assert.equal(page.onInitialized, onInitialized1);

var onInitialized2 = function() { var y = "y"; };
page.onInitialized = onInitialized2;
assert.equal(page.onInitialized, onInitialized2);
assert.notEqual(page.onInitialized, onInitialized1);

page.onInitialized = null;
// Will only allow setting to a function value, so setting it to `null` returns `undefined`
assert.equal(page.onInitialized, undefined);

page.onInitialized = undefined;
assert.equal(page.onInitialized, undefined);
