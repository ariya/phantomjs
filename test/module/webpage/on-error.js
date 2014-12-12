var assert = require('../../assert');
var page = require('webpage').create();

assert.notEqual(page.onError, undefined);

var onErrorFunc1 = function() { return !"x"; };
page.onError = onErrorFunc1;
assert.equal(page.onError, onErrorFunc1);

var onErrorFunc2 = function() { return !!"y"; };
page.onError = onErrorFunc2;
assert.equal(page.onError, onErrorFunc2);
assert.notEqual(page.onError, onErrorFunc1);

page.onError = null;
// Will only allow setting to a function value, so setting it to `null` returns `undefined`
assert.equal(page.onError, undefined);
page.onError = undefined;
assert.equal(page.onError, undefined);

// reports error
var lastError = null;
page.onError = function(message) { lastError = message; };

page.evaluate(function() { referenceError2(); });
assert.equal(lastError, "ReferenceError: Can't find variable: referenceError2");

page.evaluate(function() { throw "foo"; });
assert.equal(lastError, "foo");

page.evaluate(function() { throw Error("foo"); });
assert.equal(lastError, "Error: foo");

// don't report handled errors
var hadError = false;
var caughtError = false;

page.onError = function() { hadError = true; };
page.evaluate(function() {
    caughtError = false;

    try {
        referenceError();
    } catch(e) {
        caughtError = true;
    }
});

assert.equal(hadError, false);
assert.isTrue(page.evaluate(function() { return caughtError; }));

// even with async
page.evaluate(function() {
    setTimeout(function() { referenceError(); }, 0);
});

assert.waitFor(function() {
  return lastError == "ReferenceError: Can't find variable: referenceError";
});
