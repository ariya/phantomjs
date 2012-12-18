describe("System object", function() {
    var system = require('system');

    it("should exist", function() {
        expect(typeof system).toEqual('object');
        expect(system).toNotEqual(null);
    });

    it("should have pid property", function() {
        expect(system.hasOwnProperty('pid')).toBeTruthy();
    });

    it("should have pid as a number", function() {
        expect(typeof system.pid).toEqual('number');
    });

    it("should have pid that is an integer", function() {
        expect(system.pid).toMatch(/^\d+$/);
    });

    it("should have pid greater than 0", function() {
        expect(system.pid).toBeGreaterThan(0);
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

    it("should have stdout as object", function() {
        expect(typeof system.stdout).toEqual('object');
        expect(null == system.stdout).toBeFalsy();
    });

    it("should have stderr as object", function() {
        expect(typeof system.stderr).toEqual('object');
        expect(null == system.stderr).toBeFalsy();
    });

    it("should have stdin as object", function() {
        expect(typeof system.stdin).toEqual('object');
        expect(null == system.stdin).toBeFalsy();
    });

});
