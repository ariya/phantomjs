var assert = require('../assert');

phantom.onError = undefined;
assert.typeOf(phantom.onError, 'undefined');

var onErrorFunc1 = function() { return !"x"; };
phantom.onError = onErrorFunc1;
asssert.deepEqual(phantom.onError, onErrorFunc1);

var onErrorFunc2 = function() { return !!"y"; };
phantom.onError = onErrorFunc2;
asssert.deepEqual(phantom.onError, onErrorFunc2);
asssert.isTrue(phantom.onError != onErrorFunc1);

// Will only allow setting to a function value, so setting it to `null` returns `undefined`
phantom.onError = null;
assert.typeOf(phantom.onError, 'undefined');

phantom.onError = undefined;
assert.typeOf(phantom.onError, 'undefined');
