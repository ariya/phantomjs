describe("WebServer constructor", function() {
    it("should not exist in window", function() {
        expect(window.hasOwnProperty('WebServer')).toBeFalsy();
    });

    it("should be a function", function() {
        var WebServer = require('webserver').create;
        expect(typeof WebServer).toEqual('function');
    });
});

function checkRequest(request, response) {
    expect(typeof request).toEqual('object');
    expectHasProperty(request, 'url');
    expectHasProperty(request, 'queryString');
    expectHasProperty(request, 'method');
    expectHasProperty(request, 'httpVersion');
    expectHasProperty(request, 'statusCode');
    expectHasProperty(request, 'isSSL');
    expectHasProperty(request, 'remoteIP');
    expectHasProperty(request, 'remotePort');
    expectHasProperty(request, 'remoteUser');
    expectHasProperty(request, 'headers');
    expectHasProperty(request, 'headerName');
    expectHasProperty(request, 'headerValue');

    expect(typeof response).toEqual('object');
    expectHasProperty(response, 'statusCode');
    expectHasProperty(response, 'headers');
    expectHasFunction(response, 'setHeader');
    expectHasFunction(response, 'write');
}

describe("WebServer object", function() {
    var server = require('webserver').create();

    it("should be creatable", function() {
        expect(typeof server).toEqual('object');
        expect(server).toNotEqual(null);
    });


    it("should have objectName as 'WebServer'", function() {
        expect(server.objectName).toEqual('WebServer');
    });

    expectHasProperty(server, 'port');
    it("should have port as string", function() {
        expect(typeof server.port).toEqual('string');
    });
    it("should not listen to any port by default", function() {
        expect(server.port).toEqual("");
    });

    /* TODO:
    expectHasProperty(page, 'settings');
    it("should have non-empty settings", function() {
        expect(page.settings).toNotEqual(null);
        expect(page.settings).toNotEqual({});
    });
    */
    expectHasFunction(server, 'listenOnPort');
    expectHasFunction(server, 'newRequest');
    expectHasFunction(server, 'close');

    it("should fail to listen to blocked ports", function() {
        //NOTE: is this really blocked everywhere?
        expect(server.listen(1, function(){})).toEqual(false);
        expect(server.port).toEqual("");
    });
    it("should be able to listen to some port", function() {
        //NOTE: this can fail if the port is already being listend on...
        expect(server.listen(12345, checkRequest)).toEqual(true);
        expect(server.port).toEqual("12345");
    });

    it("should handle requests", function() {
        var page = require('webpage').create();
        var url = "http://localhost:12345/foo/bar.php?asdf=true";
        page.open(url, function (status) {
            expect(status == 'success').toEqual(true);
        });
    });
});
