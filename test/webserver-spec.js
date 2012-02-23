describe("WebServer constructor", function() {
    it("should not exist in window", function() {
        expect(window.hasOwnProperty('WebServer')).toBeFalsy();
    });

    it("should be a function", function() {
        var WebServer = require('webserver').create;
        expect(typeof WebServer).toEqual('function');
    });
});

var expectedPostData = false;

function checkRequest(request, response) {
    expect(typeof request).toEqual('object');
    expect(request.hasOwnProperty('url')).toBeTruthy();
    expect(request.hasOwnProperty('method')).toBeTruthy();
    expect(request.hasOwnProperty('httpVersion')).toBeTruthy();
    expect(request.hasOwnProperty('headers')).toBeTruthy();
    expect(typeof request.headers).toEqual('object');

    expect(typeof response).toEqual('object');
    expect(response.hasOwnProperty('statusCode')).toBeTruthy();
    expect(response.hasOwnProperty('headers')).toBeTruthy();
    expect(typeof response['setHeaders']).toEqual('function');
    expect(typeof response['setHeader']).toEqual('function');
    expect(typeof response['header']).toEqual('function');
    expect(typeof response['write']).toEqual('function');
    expect(typeof response['writeHead']).toEqual('function');

    if (expectedPostData !== false) {
        expect(request.method).toEqual("POST");
        expect(request.hasOwnProperty('post')).toBeTruthy();
        expect(typeof request.post).toEqual('object');
        console.log(JSON.stringify(request.post, null, 4));
        console.log(JSON.stringify(expectedPostData, null, 4));
        console.log(JSON.stringify(request.headersappl, null, 4));
        expect(request.post).toEqual(expectedPostData);
        expect(request.hasOwnProperty('rawData')).toBeTruthy();
        expect(typeof request.rawData).toEqual('object');
        expectedPostData = false;
    }

    response.write("request handled");
    response.close();
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
        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, function (status) {
                expect(status == 'success').toEqual(true);
                expect(page.plainText).toEqual("request handled");
                handled = true;
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
        });
    });

    it("should handle post requests", function() {
        var page = require('webpage').create();
        var url = "http://localhost:12345/foo/bar.txt?asdf=true";
        //note: sorted by key (map)
        expectedPostData = {'answer' : "42", 'universe' : "expanding"};
        var handled = false;
        runs(function() {
            expect(handled).toEqual(false);
            page.open(url, 'post', "universe=expanding&answer=42", function (status) {
                expect(status == 'success').toEqual(true);
                expect(page.plainText).toEqual("request handled");
                handled = true;
            });
        });

        waits(50);

        runs(function() {
            expect(handled).toEqual(true);
        });
    });
});
