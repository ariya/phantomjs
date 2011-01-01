if (phantom.state.length === 0) {
    if (phantom.arguments.length !== 2) {
        console.log('Usage: rasterize.js URL filename');
        phantom.exit();
    } else {
        var address = phantom.arguments[0];
        phantom.state = 'rasterize';
        phantom.viewportSize = { width: 600, height: 600 };
        phantom.open(address);
    }
} else {
    var output = phantom.arguments[1];
    phantom.sleep(200);
    phantom.render(output);
    phantom.exit();
}
