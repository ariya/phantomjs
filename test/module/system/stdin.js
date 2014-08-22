var assert = require('../../assert');
var system = require('system');

assert.typeOf(system.stdin, 'object');
assert.typeOf(system.stdin.read, 'function');
assert.typeOf(system.stdin.readLine, 'function');
assert.typeOf(system.stdin.close, 'function');
