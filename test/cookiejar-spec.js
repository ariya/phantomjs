describe("CookieJar object", function() {
    var jar = require('cookiejar').create();

    it("should be creatable", function() {
        expect(typeof jar).toEqual('object');
        expect(jar).toNotEqual(null);
    });

    expectHasProperty(jar, 'cookies');

    expectHasFunction(jar, 'addCookie');
    expectHasFunction(jar, 'deleteCookie');
    expectHasFunction(jar, 'clearCookies');

    it("should add a cookie and then remove it", function() {
        var cookie = {
            'name' : 'Valid-Cookie-Name',
            'value' : 'Valid-Cookie-Value',
            'domain' : 'localhost',
            'path' : '/foo',
            'httponly' : true,
            'secure' : false
        };

        jar.addCookie(cookie);
        var cookies = jar.cookies;

        expect(cookies.length).toEqual(1);

        expect(jar.deleteCookie('Valid-Cookie-Name')).toBe(true);

        expect(jar.cookies.length).toBe(0);
    });

    it("should set and get cookies with .cookies", function() {
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
        expect(jar.cookies.length).toBe(2);

        jar.clearCookies();
        expect(jar.cookies.length).toEqual(0);

    });

    it("should be separate cookie jars", function() {
        var jar1 = require('cookiejar').create();
        var jar2 = require('cookiejar').create();

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

        expect(jar1.cookies.length).toBe(1);
        expect(jar2.cookies.length).toBe(0);

        jar2.addCookie(cookie2);
        expect(jar1.deleteCookie('Valid-Cookie-Name-1')).toBe(true);

        expect(jar1.cookies.length).toBe(0);
        expect(jar2.cookies.length).toBe(1);

        jar1.close();
        jar2.close();


    });

});
