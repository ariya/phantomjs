var assert = require('../../assert');
var system = require('system');

assert.isTrue(system.hasOwnProperty('os'));
assert.typeOf(system.os, 'object');

assert.typeOf(system.os.architecture, 'string');
assert.typeOf(system.os.name, 'string');
assert.typeOf(system.os.version, 'string');
