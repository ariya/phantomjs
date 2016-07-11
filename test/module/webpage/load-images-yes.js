//! phantomjs: --load-images=yes
"use strict";

var url = TEST_HTTP_BASE + 'load-images.html';

async_test(function testLoadImagesYes () {
    var page = require('webpage').create();

    var numImagesRequested = 0;
    var numImagesLoaded = 0;
    
    page.onResourceRequested = this.step_func(function (resource) {
        if (/\.png$/.test(resource.url)) ++numImagesRequested;
    });

    page.onResourceReceived = this.step_func(function (resource) {
        if (resource.stage === 'end' && /\.png$/.test(resource.url)) ++numImagesLoaded;
    });

    function checkImageCounts ()
    {
        var imageCounts = page.evaluate (function countImages() {    
            "use strict";
            try
            {
                var images = document.getElementsByTagName('img');
                var imageCounts = {};
                imageCounts.numImages = images.length;
                imageCounts.numComplete = Array.prototype.reduce.call(images, function(n, image) { return n + (image.complete? 1 : 0); }, 0);
                return imageCounts;
            }
            catch(err)
            {
                return undefined;
            }
        });
        assert_equals(imageCounts.numImages, 2);
        assert_equals(imageCounts.numComplete, 2);
        assert_equals(numImagesRequested, 2);
        assert_equals(numImagesLoaded, 2);
    }
    
    page.open(url, this.step_func_done (function pageOpened(status) {
        assert_equals(status, "success");
        checkImageCounts();
        page.release();
    }));
}, "load a page two images with --load-images=yes");
