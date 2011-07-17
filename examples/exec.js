if (phantom.args.length === 0) {
    console.log('Try to pass some args when invoking this script!');
    phantom.exit();
} else {
    var argstr = '';
    phantom.args.forEach(function(arg, i) {
            argstr += arg + ' ';
    });
    phantom.exec(argstr);
}
