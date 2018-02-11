var webpage = require('webpage');

test(function () {
    var page = webpage.create();
    page.setContent('<html><body><iframe src="about:blank"></iframe></body></html>', 'http://example.com');

    page.switchToFrame(page.framesName[0]);
    var expectedContent = '<html><body><div>Test div</div></body></html>';
    var expectedLocation = 'http://www.phantomjs.org/';
    page.setFrameContent(expectedContent, expectedLocation);

    // Go back to the main page and make sure it isn't changed.
    page.switchToParentFrame();
    var actualContent = page.evaluate(function() {
        return document.documentElement.textContent;
    });
    assert_not_equals(actualContent, 'Test div');

    var actualLocation = page.evaluate(function() {
        return window.location.href;
    });
    assert_not_equals(actualLocation, expectedLocation);

    // Go to the iframe and make sure it changed.
    page.switchToFrame(page.framesName[0]);
    var actualContent = page.evaluate(function() {
        return document.documentElement.textContent;
    });
    assert_equals(actualContent, 'Test div');

    var actualLocation = page.evaluate(function() {
        return window.location.href;
    });
    assert_equals(actualLocation, expectedLocation);

}, "manually set iframe page content and location");
