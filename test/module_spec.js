describe("Module", function() {
    it("has filename property containing its absolute path", function() {
        module.filename.should.match(/\/.*test\/module_spec.js/);
    });

    it("has id property equal to filename", function() {
        module.id.should.equal(module.filename);
    });

    it("has dirname property containing absolute path to its directory", function() {
        module.dirname.should.match(/\/.*test/);
    });

    it("its require() can be used externally", function() {
        var exposed = require('dummy_exposed');
        exposed.require('./dummy_file').should.equal('spec/node_modules/dummy_file');
    });
});
