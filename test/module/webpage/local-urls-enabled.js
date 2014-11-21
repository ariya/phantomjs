// phantomjs: --local-url-access=yes

var assert = require("../../assert");
var p = require("webpage").create();

var url = "file:///nonexistent";

var rsErrorCalled = false;
p.onResourceError = function (error) {
    rsErrorCalled = true;
    assert.strictEqual(error.url, url);
    assert.strictEqual(error.errorCode, 203);
    assert.strictEqual(/^Error opening\b.*?\bnonexistent:/.test(error.errorString),
                       true);
};

p.open(url, function () {
    assert(rsErrorCalled);
});
