var assert = require('../../assert');
var cookiejar = require('cookiejar');

var jar1 = cookiejar.create();
var jar2 = cookiejar.create();

var cookie1 = {
    'name' : 'Valid-Cookie-Name-1',
    'value' : 'Valid-Cookie-Value',
    'domain' : 'localhost',
    'path' : '/foo',
    'httponly' : true,
    'secure' : false
};

var cookie2 = {
    'name' : 'Valid-Cookie-Name-2',
    'value' : 'Valid-Cookie-Value',
    'domain' : 'localhost',
    'path' : '/foo',
    'httponly' : true,
    'secure' : false
};

jar1.addCookie(cookie1);
assert.equal(jar1.cookies.length, 1);
assert.equal(jar2.cookies.length, 0);

jar2.addCookie(cookie2);
jar1.deleteCookie('Valid-Cookie-Name-1');
assert.equal(jar1.cookies.length, 0);
assert.equal(jar2.cookies.length, 1);

jar1.close();
jar2.close();
