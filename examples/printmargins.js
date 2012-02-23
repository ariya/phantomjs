var page = require('webpage').create();

if (phantom.args.length < 6) {
    console.log('Usage: printmargins.js URL filename LEFT TOP RIGHT BOTTOM');
    console.log('  margin examples: "1cm", "10px", "7mm", "5in"');
    phantom.exit();
} else {
    var address = phantom.args[0];
    var output = phantom.args[1];
    var marginLeft = phantom.args[2];
    var marginTop = phantom.args[3];
    var marginRight = phantom.args[4];
    var marginBottom = phantom.args[5];
    page.viewportSize = { width: 600, height: 600 };
    page.paperSize = {
        format: 'A4',
        margin: {
            left: marginLeft,
            top: marginTop,
            right: marginRight,
            bottom: marginBottom
        }
    };
    page.open(address, function (status) {
        if (status !== 'success') {
            console.log('Unable to load the address!');
        } else {
            window.setTimeout(function () {
                page.render(output);
                phantom.exit();
            }, 200);
        }
    });
}
