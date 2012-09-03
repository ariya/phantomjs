describe("System object", function() {
    var system = require('system');

    it("should exist", function() {
        expect(typeof system).toEqual('object');
        expect(system).toNotEqual(null);
    });

    it("should have platform as string", function() {
        expect(typeof system.platform).toEqual('string');
    });

    it("should have platform set to 'phantomjs'", function() {
        expect(system.platform).toEqual('phantomjs');
    });

    it("should have args as array", function() {
        expect(system.args instanceof Array).toBeTruthy();
    });

    it("should have args with at least one item", function() {
        expect(system.args.length >= 1).toBeTruthy();
    });

    it("should have args[0] as the this test runner", function() {
        expect(system.args[0]).toMatch(/run-tests.js$/);
    });

    it("should have env as object", function() {
        expect(typeof system.env).toEqual('object');
    });

    it("should have os as object", function() {
        expect(typeof system.os).toEqual('object');
    });

    it("should have isSSLSupported as boolean", function() {
        expect(typeof system.isSSLSupported).toEqual('boolean');
    });

});
