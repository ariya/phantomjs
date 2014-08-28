var assert = require('../../assert');
var system = require('system');

assert.isTrue(system.hasOwnProperty('pid'));

// pid must be a positive integer
assert.typeOf(system.pid, 'number');
assert.isTrue(system.pid > 0);
