describe("WebServer constructor", function() {
    it("should exist in window", function() {
        expect(window.hasOwnProperty('WebServer')).toBeTruthy();
    });

    it("should be a function", function() {
        expect(typeof window.WebServer).toEqual('function');
    });
});

describe("WebServer object", function() {
    var server = new WebServer();

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
});
