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
});
