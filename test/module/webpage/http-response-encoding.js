// Basic test for proper encoding of response status lines and header values.
// See https://github.com/ariya/phantomjs/issues/12758

var assert = require("../../assert");
var p = require("webpage").create();

var url = "http://localhost:9180/non-ascii-response-headers";
var rrCalled = false;
var reCalled = false;
p.onResourceReceived = function (response) {
    var i, found = false;
    for (i = 0; i < response.headers.length; i++) {
        if (response.headers[i].name === "Set-Cookie") {
            found = true;
            assert.strictEqual(response.headers[i].value,
                               "κουλουράκι"); // Greek: "cookie, pretzel"
            break;
        }
    }
    assert.isTrue(found);

    if (response.url === url) {
        rrCalled = true;
        assert.strictEqual(response.status, 200);
        assert.strictEqual(response.statusText, "行"); // Chinese: "OK" (srsly)
    } else {
        assert.strictEqual(response.url, url + "?1");
        assert.strictEqual(response.status, 404);
        assert.strictEqual(response.statusText, "不存在"); // Chinese: "does not exist"
    }
};

p.onResourceError = function (err) {
    reCalled = true;
    assert.strictEqual(err.url, url + "?1");
    assert.strictEqual(err.errorCode, 203);
    assert.strictEqual(err.status, 404);
    assert.strictEqual(err.statusText, "不存在"); // Chinese: "does not exist"
};

p.open(url, function () {
    assert.isTrue(rrCalled);
    assert.isTrue(reCalled);
});

