const webpage = require('webpage');

test(function () {
    const page = webpage.create();

    page.devicePixelRatio = 1.5;
    assert_equals(page.devicePixelRatio, 1.5);

    page.devicePixelRatio = 2.0;
    assert_equals(page.devicePixelRatio, 2.0);

    page.devicePixelRatio = 0.5;
    assert_equals(page.devicePixelRatio, 0.5);
}, "page.devicePixelRatio");
