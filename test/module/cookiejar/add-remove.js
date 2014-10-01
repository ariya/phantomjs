var assert = require('../../assert');
var cookiejar = require('cookiejar');

var jar = cookiejar.create();
assert.equal(jar.cookies.length, 0);

var cookie = {
    'name' : 'Valid-Cookie-Name',
    'value' : 'Valid-Cookie-Value',
    'domain' : 'localhost',
    'path' : '/foo',
    'httponly' : true,
    'secure' : false
};

jar.addCookie(cookie);
assert.equal(jar.cookies.length, 1);

jar.deleteCookie('Valid-Cookie-Name');
assert.equal(jar.cookies.length, 0);
