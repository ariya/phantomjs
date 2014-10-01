var assert = require('../../assert');
var cookiejar = require('cookiejar');

var jar = cookiejar.create();

assert.typeOf(jar.addCookie, 'function');
assert.typeOf(jar.deleteCookie, 'function');
assert.typeOf(jar.clearCookies, 'function');
