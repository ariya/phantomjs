
describe("Canvas toDataURL PNG encoding", function(){
    var TEST_FILE_DIR = "canvas-spec-renders/";

    var p = require("webpage").create();
    var fs = require("fs");

    p.onConsoleMessage = function(m) { console.log(m); };

    function to_data_url_test(use_format) {
        p.open(TEST_FILE_DIR + "canvas-test.html", function(status) {

            var base64 = p.evaluate(function(use_format) {
                var image = document.getElementById("puppy");

                var canvas = document.createElement("canvas");
                canvas.width = image.width;
                canvas.height = image.height;

                var context = canvas.getContext("2d");
                context.drawImage(image, 0, 0);

                if(!use_format) {
                    return canvas.toDataURL();
                } else {
                    return canvas.toDataURL("image/png");
                }
            }, use_format);

            var image_size_oom = Math.round(Math.log(fs.size(TEST_FILE_DIR + "puppy.png")) / Math.log(10));
            var base64_size_oom = Math.round(Math.log(base64.length) / Math.log(10));

            //The orders of magnitude should be the same. Prior to the fix, the encoded base64 was much larger
            expect(image_size_oom).toEqual(base64_size_oom);
        });
    }

    it("Should properly encode PNG base64 without any arguments", function() {
        to_data_url_test(false);
        waits(1000);
    });

    it("Should properly encode PNG base64 with explicit image/png argument", function(){
        to_data_url_test(true);
        waits(1000);
    });
});
