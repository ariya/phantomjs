var assert = require('../../assert');
var system = require('system');

assert.typeOf(system, 'object');
assert.isTrue(system !== null);

assert.isTrue(system.hasOwnProperty('platform'));
assert.typeOf(system.platform, 'string');
assert.equal(system.platform, 'phantomjs');

assert.isTrue(system.hasOwnProperty('env'));
assert.typeOf(system.env, 'object');

assert.isTrue(system.hasOwnProperty('isSSLSupported'));
assert.typeOf(system.isSSLSupported, 'boolean');
assert.equal(system.isSSLSupported, true);
