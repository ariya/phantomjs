var webpage = require('webpage');

test(function () {
    var defaultPage = webpage.create();
    assert_deep_equals(defaultPage.devicePixelRatio, 1.0);
}, "default device pixel ratio");

test(function () {
    var options = {
        devicePixelRatio: 1.5
    };
    var customPage = webpage.create(options);
    assert_deep_equals(customPage.devicePixelRatio, options.devicePixelRatio);
}, "custom device pixel ratio");
