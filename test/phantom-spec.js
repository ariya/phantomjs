describe("phantom global object", function() {
    it("should exist", function() {
        expect(typeof phantom).toEqual('object');
    });

    it("should have args property", function() {
        expect(phantom.hasOwnProperty('args')).toBeTruthy();
    });

    it("should have args as an array", function() {
        expect(typeof phantom.args).toEqual('object');
    });

    it("should have libraryPath property", function() {
        expect(phantom.hasOwnProperty('libraryPath')).toBeTruthy();
    });

    it("should have libraryPath as a string", function() {
        expect(typeof phantom.libraryPath).toEqual('string');
    });

    it("should not have an empty libraryPath", function() {
        expect(phantom.libraryPath.length).toNotEqual(0);
    });

    it("should have scriptName property", function() {
        expect(phantom.hasOwnProperty('scriptName')).toBeTruthy();
    });

    it("should have scriptName as a string", function() {
        expect(typeof phantom.scriptName).toEqual('string');
    });

    it("should not have an empty scriptName", function() {
        expect(phantom.scriptName.length).toNotEqual(0);
    });

    it("should have outputEncoding property", function() {
        expect(phantom.hasOwnProperty('outputEncoding')).toBeTruthy();
    });

    it("should have the default outputEncoding of UTF-8", function() {
        expect(phantom.outputEncoding.toLowerCase()).toEqual('utf-8');
    });

    it("should have version property", function() {
        expect(phantom.hasOwnProperty('version')).toBeTruthy();
    });

    it("should return 1 as the major version", function() {
        expect(phantom.version.major).toEqual(1);
    });

    it("should return 9 as the minor version", function() {
        expect(phantom.version.minor).toEqual(9);
    });

    it("should return 8 as the patch version", function() {
        expect(phantom.version.patch).toEqual(8);
    });

    it("should have 'injectJs' function", function() {
        expect(typeof phantom.injectJs).toEqual("function");
    });

    it("should have 'exit' function", function() {
        expect(typeof phantom.exit).toEqual("function");
    });

    it("should have 'cookiesEnabled' property, and should be 'true' by default", function() {
        expect(phantom.hasOwnProperty('cookiesEnabled')).toBeTruthy();
        expect(phantom.cookiesEnabled).toBeTruthy();
    });

    it("should be able to get the error signal handler that is currently set on it", function() {
        phantom.onError = undefined;
        expect(phantom.onError).toBeUndefined();
        var onErrorFunc1 = function() { return !"x"; };
        phantom.onError = onErrorFunc1;
        expect(phantom.onError).toEqual(onErrorFunc1);
        var onErrorFunc2 = function() { return !!"y"; };
        phantom.onError = onErrorFunc2;
        expect(phantom.onError).toEqual(onErrorFunc2);
        expect(phantom.onError).toNotEqual(onErrorFunc1);
        phantom.onError = null;
        // Will only allow setting to a function value, so setting it to `null` returns `undefined`
        expect(phantom.onError).toBeUndefined();
        phantom.onError = undefined;
        expect(phantom.onError).toBeUndefined();
    });
});
