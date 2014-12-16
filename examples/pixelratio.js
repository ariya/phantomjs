var page = require('webpage').create(),
	address, output, pixelRatio;

if (phantom.args.length < 3 || phantom.args.length > 4) {
    console.log('Usage: viewport.js URL filename pixelRatio');
    phantom.exit();
} else {
    address = phantom.args[0];
    output = phantom.args[1];
    pixelRatio = phantom.args[2];
    page.viewportSize = { width: (1280*pixelRatio), height: (800*pixelRatio) };
    page.open(address, function (status) {
        if (status !== 'success') {
            console.log('Unable to load the address!');
        } else {
            page.evaluate(function (s) {
                /* scale the whole body */
                document.body.style.webkitTransform = "scale(" + s + ")";
                document.body.style.webkitTransformOrigin = "0% 0%";
                /* fix the body width that overflows out of the viewport */
                document.body.style.width = (100 / s) + "%";
            }, pixelRatio);
            window.setTimeout(function () {
                page.render(output);
                phantom.exit();
            }, 200);
        }
    });
}