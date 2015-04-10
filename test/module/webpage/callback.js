var assert = require('../../assert');
var page = require('webpage').create();

var msgA = "a",
    msgB = "b",
    result,
    expected = msgA + msgB;
page.onCallback = function(a, b) {
    return a + b;
};
result = page.evaluate(function(a, b) {
    return callPhantom(a, b);
}, msgA, msgB);

assert.equal(result, expected);
