var assert = require('../../assert');
var system = require('system');

assert.typeOf(system.stdout, 'object');
assert.typeOf(system.stdout.write, 'function');
assert.typeOf(system.stdout.writeLine, 'function');
assert.typeOf(system.stdout.flush, 'function');
assert.typeOf(system.stdout.close, 'function');
