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


function render_buffer_test(format, quality) {

    var expect_content = renders.get(format, quality || "");

    var p = webpage.create();



    p.paperSize = { width: '300px', height: '300px', border: '0px' };
    p.clipRect = { top: 0, left: 0, width: 300, height: 300};
    p.viewportSize = { width: 300, height: 300};

    p.open(TEST_HTTP_BASE + "render/", this.step_func_done(function (status) {
        var content = p.renderBuffer(format, quality|| -1);

        // Go from a Uint8ClampedArray to an array of bytes that we can compare with data from disk
        var byte_content = [].map.call(content, function(v){
            return String.fromCharCode(v);
        }).join("");

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
        assert_is_true(byte_content == expect_content);
    }));
}


[
    // TODO when PDF is implemented
    //["PDF",                               "pdf", {}],
    //["PDF (format option)",               "pdf", {format: "pdf"}],
    ["PNG to Buffer",               "png",],
    ["JPEG to Buffer",              "jpg"],
    ["JPEG Buffer (quality option)",             "jpg", 50],
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
    async_test(function () { render_buffer_test.call(this, format, quality); },
               label, props);
});
