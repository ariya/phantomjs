if (phantom.args.length === 0) {
    console.log('Try to pass some args when invoking this script!');
} else {
    phantom.args.forEach(function (arg, i) {
            console.log(i + ': ' + arg);
    });
}
phantom.exit();
