var fs      = require("fs");
var system  = require("system");
var webpage = require("webpage");
var renders = require("./renders");

function clean_pdf(data) {
    // FIXME: This is not nearly enough normalization.
    data = data.replace(/\/(Title|Creator|Producer|CreationDate) \([^\n]*\)/g, "/$1 ()");
    data = data.replace(/\nxref\n[0-9 nf\n]+trailer\b/, "\ntrailer");
    data = data.replace(/\nstartxref\n[0-9]+\n%%EOF\n/, "\n");
    return data;
}

function render_test(format) {
    var expect_content = btoa(renders.get(format, ""));
    var p = webpage.create();

    p.paperSize = { width: '300px', height: '300px', border: '0px' };
    p.clipRect = { top: 0, left: 0, width: 300, height: 300};
    p.viewportSize = { width: 300, height: 300};

    p.open(TEST_HTTP_BASE + "render/", this.step_func_done(function (status) {
        var content = p.renderBase64(format);

        // expected variation in PDF output
        if (format === "pdf") {
            content = clean_pdf(content);
            expect_content = clean_pdf(expect_content);
        }

        // Don't dump entire images to the log on failure.
        assert_is_true(content === expect_content);
    }));
}

[
    "pdf",
    "png",
    "jpg",
]
.forEach(function (format) {
    var props  = {};

    // All tests fail on Linux.  All tests except JPG fail on Mac.
    // Currently unknown which tests fail on Windows.
    if (format !== "jpg" || system.os.name !== "mac")
        props.expected_fail = true;

    async_test(function () { render_test.call(this, format); },
               format, props);
});
