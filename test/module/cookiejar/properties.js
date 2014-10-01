var assert = require('../../assert');
var cookiejar = require('cookiejar');

var jar = cookiejar.create();

assert.typeOf(jar, 'object');
assert.isTrue(jar !== null);
assert.typeOf(jar.cookies, 'object');
