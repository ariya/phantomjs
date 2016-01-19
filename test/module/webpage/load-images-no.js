//! phantomjs: --load-images=no

// Compiled for debug, this tests for leaks when not loading images.
//

var url = TEST_HTTP_BASE + 'load-images-no.html';
function do_test() {
    var page = require('webpage').create();

    page.open(url, this.step_func_done (function (status) {
        assert_equals(status, "success");
        page.release();
    }));
}

async_test(do_test, "load a page two images with --load-images=no");
