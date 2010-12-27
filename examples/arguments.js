if (phantom.arguments.length === 0) {
    phantom.log('Try to pass some arguments when invoking this script!');
} else {
    phantom.arguments.forEach(function (arg, i) {
            phantom.log(i + ': ' + arg);
    });
}
phantom.exit();
