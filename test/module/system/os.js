var assert = require('../../assert');
var system = require('system');

assert.isTrue(system.hasOwnProperty('os'));
assert.typeOf(system.os, 'object');

assert.typeOf(system.os.architecture, 'string');
assert.typeOf(system.os.name, 'string');
assert.typeOf(system.os.version, 'string');

if (system.os.name === 'mac') {
    // release is x.y.z with x = 10 for Snow Leopard and 14 for Yosemite
    assert.typeOf(system.os.release, 'string');
    assert.isTrue(parseInt(system.os.release, 10) >= 10);
}
