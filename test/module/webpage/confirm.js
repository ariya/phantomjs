var assert = require('../../assert');
var page = require('webpage').create();

var msg = "message body",
    result,
    expected = true;
page.onConfirm = function(msg) {
    return true;
};
result = page.evaluate(function(m) {
    return window.confirm(m);
}, msg);

assert.equal(result, expected);
