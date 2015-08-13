async_test(function () {
    var webpage = require('webpage');
    var page = webpage.create();
    assert_type_of(page, 'object');

    page.open('http://localhost:9180/hello.html',
              this.step_func_done(function (status) {
                  assert_equals(status, 'success');
                  assert_type_of(page.title, 'string');
                  assert_equals(page.title, 'Hello');
                  assert_type_of(page.plainText, 'string');
                  assert_equals(page.plainText, 'Hello, world!');
              }));
}, "page.open");
