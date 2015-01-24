var assert = require('../assert');

assert.isTrue(phantom.hasOwnProperty('version'));

assert.typeOf(phantom.version, 'object');
assert.typeOf(phantom.version.major, 'number');
assert.typeOf(phantom.version.minor, 'number');
assert.typeOf(phantom.version.patch, 'number');

assert.equal(phantom.version.major, 2);
assert.equal(phantom.version.minor, 0);
assert.equal(phantom.version.patch, 1);
