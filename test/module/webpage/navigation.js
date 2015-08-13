async_test(function () {
    var page = require("webpage").create();
    var url1 = "http://localhost:9180/navigation/index.html";
    var url2 = "http://localhost:9180/navigation/dest.html";

    var onLoadFinished1 = this.step_func(function (status) {
        assert_equals(status, "success");
        assert_equals(page.url, url1);
        assert_equals(page.evaluate(function () {
            return document.body.innerHTML;
        }), "INDEX\n");

        page.onLoadFinished = onLoadFinished2;
        page.evaluate(function() {
            window.location = "dest.html";
        });
    });

    var onLoadFinished2 = this.step_func_done(function (status) {
        assert_equals(status, "success");
        assert_equals(page.url, url2);
        assert_equals(page.evaluate(function () {
            return document.body.innerHTML;
        }), "DEST\n");
    });

    page.onLoadFinished = onLoadFinished1;
    page.open(url1);

}, "navigating to a relative URL using window.location");
