describe("WebKit", function() {
    it("should not crash when failing to dirty lines while removing a inline.", function () {
        var p = require("webpage").create();
        p.open('../test/webkit-spec/inline-destroy-dirty-lines-crash.html');
        waits(50);
    });
});
