if (phantom.args.length === 0) {
    console.log("This example instruct PhantomJS to just open a specific URL.")
    console.log('URL must be passed as a parameter.');
    phantom.exit();
} else {
    if (phantom.state.length === 0) {
        console.log("Loading: " + phantom.args[0]);
        phantom.state = 'gotourl';
        phantom.userAgent = "PhantomJS";
        phantom.open(phantom.args[0]);
    } else {
        // TODO - I don't have a specific condition for exit yet
    }
}
