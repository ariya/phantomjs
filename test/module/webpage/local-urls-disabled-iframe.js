// phantomjs: --web-security=no --local-url-access=no

var assert = require("../../assert");
var p = require("webpage").create();

var url = "http://localhost:9180/iframe.html#file:///nonexistent";

var rsErrorCalled = false;
p.onResourceError = function (error) {
    rsErrorCalled = true;
    assert.strictEqual(error.url, "file:///nonexistent");
    assert.strictEqual(error.errorCode, 301);
    assert.strictEqual(error.errorString, 'Protocol "file" is unknown');
};

p.open(url, function () {
    assert.isTrue(rsErrorCalled);
});
