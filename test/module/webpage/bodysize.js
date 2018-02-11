async_test(function () {
    var page = require('webpage').create();

    // Do NOT capture any body.
    page.captureContent = [];

    var size = 0;
    page.onResourceReceived = function (req) {
        if (req.stage == 'end') {
            size = req.bodySize;
        }
    };

    page.open(TEST_HTTP_BASE + "bodysize", this.step_func_done(function() {
        assert_equals(size, 8);
    }));

}, "bodysize");
