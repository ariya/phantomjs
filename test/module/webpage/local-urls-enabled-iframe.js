//! phantomjs: --web-security=no --local-url-access=yes

var webpage = require("webpage");
var system = require('system');

async_test(function () {
    var page = webpage.create();
    var url = TEST_HTTP_BASE + "iframe.html#file:///really-nonexistent";
    var rsErrorCalled = false;

    page.onResourceError = this.step_func(function (error) {
        rsErrorCalled = true;
        assert_equals(error.url, "file:///really-nonexistent");

        assert_equals(error.errorCode, 203);
        assert_regexp_match(error.errorString,
                            /^Error opening\b.*?\breally-nonexistent:/);
    });

    page.open(url, this.step_func_done(function () {
        assert_is_true(rsErrorCalled);
    }));

}, "attempts to load a file: URL in an iframe with --local-url-access=yes");
