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
});
