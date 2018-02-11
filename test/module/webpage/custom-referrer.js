async_test(function () {
    var webpage = require('webpage');
    var page = webpage.create();

    // Set Custom Refer(r)er by using customHeaders
    // See https://en.wikipedia.org/wiki/HTTP_referer in case you wonder about the spelling.
    page.customHeaders =  {
        'Referer': 'http://example.com'
    };

    page.open(TEST_HTTP_BASE + 'echo', this.step_func_done(function (status) {
        
        // Check whether document.referrer returns the custom referrer as set above using a custom Header.
        var document_referrer = page.evaluate(function () {
                return document.referrer;
        });

        assert_equals(document_referrer, 'http://example.com');

    }));

    

}, "A custom referrer as set with page.customHeaders should be accessible using document.referrer");