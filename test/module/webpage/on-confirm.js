var assert = require('../../assert');
var page = require('webpage').create();

var msg = "message body",
    result,
    expected = true;

assert.equal(page.onConfirm, undefined);

var onConfirmTrue = function(msg) {
    return true;
};
page.onConfirm = onConfirmTrue;
assert.equal(page.onConfirm, onConfirmTrue);

result = page.evaluate(function(m) {
    return window.confirm(m);
}, msg);

assert.equal(result, expected);

var onConfirmFunc = function() { return !!"y"; };
page.onConfirm = onConfirmFunc2;
assert.equal(page.onConfirm, onConfirmFunc);
assert.notEqual(page.onConfirm, onConfirmTrue);

page.onConfirm = null;
// Will only allow setting to a function value, so setting it to `null` returns `undefined`
assert.equal(page.onConfirm, undefined);
page.onConfirm = undefined;
assert.equal(page.onConfirm, undefined);
