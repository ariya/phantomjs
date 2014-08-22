var assert = require('../../assert');
var system = require('system');

assert.typeOf(system.stderr, 'object');
assert.typeOf(system.stderr.write, 'function');
assert.typeOf(system.stderr.writeLine, 'function');
assert.typeOf(system.stderr.flush, 'function');
assert.typeOf(system.stderr.close, 'function');
