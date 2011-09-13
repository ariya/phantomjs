describe("module loading using require", function() {

    it("should work for 'webpage' module", function() {
        expect(typeof require('webpage')).toEqual('object');
    });

    it("should work for 'fs' module", function() {
        expect(typeof require('fs')).toEqual('object');
    });

    it("should throw an error for an unknown module", function() {
        var module = 'foobar';
        expect(function(){
            var foo = require(module);
        }).toThrow("Unknown module " + module + " for require()");
    });

});
