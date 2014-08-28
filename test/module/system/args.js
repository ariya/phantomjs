var assert = require('../../assert');
var system = require('system');

assert.isTrue(system.hasOwnProperty('args'));
assert.typeOf(system.args, 'object');
assert.isTrue(system.args instanceof Array);
assert.isTrue(system.args.length >= 1);

// args[0] should be this script itself
assert.isTrue(system.args[0].match(/args\.js$/));
