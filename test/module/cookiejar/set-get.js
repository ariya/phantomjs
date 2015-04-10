var assert = require('../../assert');
var cookiejar = require('cookiejar');

var jar = cookiejar.create();
assert.equal(jar.cookies.length, 0);

var cookies = [{
    'name' : 'Valid-Cookie-Name',
    'value' : 'Valid-Cookie-Value',
    'domain' : 'localhost',
    'path' : '/foo',
    'httponly' : true,
    'secure' : false
},{
    'name' : 'Valid-Cookie-Name-Sec',
    'value' : 'Valid-Cookie-Value-Sec',
    'domain' : 'localhost',
    'path' : '/foo',
    'httponly' : true,
    'secure' : false,
    'expires' : new Date().getTime() + 3600 //< expires in 1h
}];

jar.cookies = cookies;
assert.equal(jar.cookies.length, 2);

jar.clearCookies();
expect(jar.cookies.length).toEqual(0);
