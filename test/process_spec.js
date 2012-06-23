describe("process", function() {
  it("has argv containing script name and arguments", function() {
    process.argv[0].should.equal('nodify');
    process.argv[1].should.match(/.*\/support\/mocha.js$/);
    process.argv[2].should.equal('dummy_arg');
  });
});
