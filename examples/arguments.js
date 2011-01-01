if (phantom.arguments.length === 0) {
    console.log('Try to pass some arguments when invoking this script!');
} else {
    phantom.arguments.forEach(function (arg, i) {
            console.log(i + ': ' + arg);
    });
}
phantom.exit();
