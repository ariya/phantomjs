/* Test the test server itself. */

var webpage = require('webpage');

function test_one_page(url) {
    async_test(function () {
        var page = webpage.create();
        page.onResourceReceived = this.step_func(function (response) {
            assert_equals(response.status, 200);
        });
        page.onResourceError = this.unreached_func();
        page.onResourceTimeout = this.unreached_func();
        page.onLoadFinished = this.step_func_done(function (status) {
            assert_equals(status, 'success');
        });
        page.open(url);
    }, url);
}

[
    'http://localhost:9180/hello.html',
    'http://localhost:9180/status?200',
    'http://localhost:9180/echo'
]
    .forEach(test_one_page);
