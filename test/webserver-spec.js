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

    it("should fail to listen to blocked ports", function() {
        //NOTE: is this really blocked everywhere?
        expect(server.listen(1, function(){})).toEqual(false);
        expect(server.port).toEqual("");
    });
    it("should be able to listen to some port", function() {
        //NOTE: this can fail if the port is already being listend on...
        expect(server.listen(12345, function() {})).toEqual(true);
        expect(server.port).toEqual("12345");
    });
});
