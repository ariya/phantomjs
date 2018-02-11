var server, port, request_cb;
setup(function () {
    server = require("webserver").create();

    // Should be unable to listen on port 1 (FIXME: this might succeed if
    // the test suite is being run with root privileges).
    assert_is_false(server.listen(1, function () {
    }));
    assert_equals(server.port, "");

    // Find an unused port in the 1024--32767 range on which to run the
    // rest of the tests.  The function in "request_cb" will be called
    // for each request; it is set appropriately by each test case.
    for(var i = 1024; i < 32768; i++) {
        if(server.listen(i, function (rq, rs) {
                return request_cb(rq, rs);
            })) {
            assert_equals(server.port, i.toString());
            port = server.port;
            return;
        }
    }
    assert_unreached("unable to find a free TCP port for server tests");
}, {"test_timeout": 1000});

function arm_check_request(test, expected_postdata, expected_bindata, expected_mimetype) {
    request_cb = test.step_func(function check_request(request, response) {

        try {
            assert_type_of(request, "object");
            assert_own_property(request, "url");
            assert_own_property(request, "method");
            assert_own_property(request, "httpVersion");
            assert_own_property(request, "headers");
            assert_type_of(request.headers, "object");

            assert_type_of(response, "object");
            assert_own_property(response, "statusCode");
            assert_own_property(response, "headers");
            assert_type_of(response.setHeaders, "function");
            assert_type_of(response.setHeader, "function");
            assert_type_of(response.header, "function");
            assert_type_of(response.write, "function");
            assert_type_of(response.writeHead, "function");

            response.setHeader("X-Request-URL", request.url);

            var sourcePage = require('webpage').create();

            sourcePage.paperSize = {width: '300px', height: '300px', border: '0px'};
            sourcePage.clipRect = {top: 0, left: 0, width: 300, height: 300};
            sourcePage.viewportSize = {width: 300, height: 300};
            sourcePage.open(TEST_HTTP_BASE + "render/", this.step_func(function start(status) {
                var a = response;

                // Buffer is an Uint8ClampedArray
                var buffer = sourcePage.renderBuffer("jpg", 75);

                response.statusCode = 200;
                response.setHeader("Content-Type", "image/jpg");

                // Pass the Buffer to 'write' to send Uint8ClampedArray
                response.write(buffer);
                response.close();
                request_cb = test.unreached_func;

            }));

        } catch(e) {
            require("system").stderr.write(e);
        } finally {
        }
    });
}

async_test(function () {
    var page = require("webpage").create();
    var test = this;

    // We should be expecting an image equivalent to one from the renders directory
    var expectedSize = require("fs").read("./module/webpage/renders/test.jpg", "b").length;

    page.onError = test.unreached_func;
    page.onResourceError = test.unreached_func;

    // Check we're getting an image of the appropriate size
    page.onResourceReceived = this.step_func(function (response) {
        if(response.stage == "start") {
            assert_equals(response.bodySize, expectedSize);
            assert_equals(response.contentType, "image/jpg");
        }
    });

    var url = "http://localhost:" + port + "/";
    arm_check_request(this, false, null, "image/jpg");
    page.open(url, "get", this.step_func_done(function (status) {
        test.done();
    }));

}, "returning data from a renderBuffer call", {
    skip: false,
    expected_fail: false
});

