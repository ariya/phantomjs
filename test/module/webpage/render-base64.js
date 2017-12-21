var fs      = require("fs");
var system  = require("system");
var webpage = require("webpage");
var renders = require("./renders");

<<<<<<< HEAD
function render_test(format, quality) {
    // Normalizes the expected quality level when out of range values are given.
    // However, the native call to PhantomJS still gets undefined, < 0 or > 100.
    var expected_quality = quality;
    if (typeof expected_quality === "undefined") {
        expected_quality = "";
    } else {
        if (expected_quality <= 0) {
            expected_quality = 1;
        } else if (expected_quality > 100) {
            expected_quality = 100;
        }
    }

    var expected_content = renders.get(format, expected_quality);

    // btoa converts the raw expected content, read from the binary image files, to base64.
    var expected_base64 = window.btoa(expected_content);

=======
function clean_pdf(data) {
    // FIXME: This is not nearly enough normalization.
    data = data.replace(/\/(Title|Creator|Producer|CreationDate) \([^\n]*\)/g, "/$1 ()");
    data = data.replace(/\nxref\n[0-9 nf\n]+trailer\b/, "\ntrailer");
    data = data.replace(/\nstartxref\n[0-9]+\n%%EOF\n/, "\n");
    return data;
}

function render_base64_test(format, quality) {
    var expect_content = renders.get(format, quality || "");
>>>>>>> 27b5754ae0bb142a2129d4ed8252d39b9140edda
    var p = webpage.create();

    p.paperSize = { width: '300px', height: '300px', border: '0px' };
    p.clipRect = { top: 0, left: 0, width: 300, height: 300};
    p.viewportSize = { width: 300, height: 300};

    p.open(TEST_HTTP_BASE + "render/", this.step_func_done(function (status) {
<<<<<<< HEAD
	// If quality isn't specified then it defaults to undefined in JavaScript and -1 in C++ code,
	// causing the default quality to be 75, like in the render function.
        var actual_base64 = p.renderBase64(format, quality);

	// Checks if the test should pass or not.
        assert_is_true(actual_base64 === expected_base64);
    }));
}

[
    ["JPEG",                              "jpg"],      // should return base64 encoding with default quality
    ["JPEG (0 quality option)",           "jpg", 0],   // should return base64 encoding as if quality was 1
    ["JPEG (1 quality option)",           "jpg", 1],
    ["JPEG (50 quality option)",          "jpg", 50],
    ["JPEG (100 quality option)",         "jpg", 100],
    ["JPEG (101 quality option)",         "jpg", 101], // should return base64 encoding as if quality was 100
]
.forEach(function (arr) {
    var label   = arr[0];
    var format  = arr[1];
    var quality = arr[2];

    async_test(function () { render_test.call(this, format, quality); },
               label);
});
=======
        var content = p.renderBase64(format, quality);

        // COnvert to a byte array
        var byte_content = atob(content);

        // Can write to fs for debugging
        //var scratch = "temp_render." + format;
        //fs.write(scratch, byte_content, "wb");

        // TODO - when pdf is implemented, deal with differences.
        // expected variation in PDF output
        //if (format === "pdf") {
        //    content = clean_pdf(content);
        //    expect_content = clean_pdf(expect_content);
        //}

        assert_equals(byte_content.length, expect_content.length)
        assert_equals(byte_content, expect_content);
    }));
}


[
    //["PDF",                               "pdf", {}],
    //["PDF (format option)",               "pdf", {format: "pdf"}],
    ["PNG to Base64",               "png"],
    ["JPEG to Base64",              "jpg"],
    ["JPEG Base64 (quality option)",             "jpg", 50],
]
.forEach(function (arr) {
    var label  = arr[0];
    var format = arr[1];
    var quality    = arr[2];
    var props  = {};

    //// All tests fail on Linux.  All tests except JPG fail on Mac.
    //// Currently unknown which tests fail on Windows.
    if (format !== "jpg" || system.os.name !== "mac")
        props.expected_fail = true;

    async_test(function () { render_base64_test.call(this, format, quality); },
               label, props);
});



>>>>>>> 27b5754ae0bb142a2129d4ed8252d39b9140edda
