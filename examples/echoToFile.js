// echoToFile.js - Write in a given file all the parameters passed on the CLI

if (phantom.args.length < 2) {
    console.log("Usage: echoToFile.js DESTINATION_FILE <arguments to echo...>");
    phantom.exit();
} else {
    var content = '',
        f = null;
    for ( i= 1; i < phantom.args.length; ++i ) {
        content += phantom.args[i] + (i === phantom.args.length-1 ? '' : ' ');
    }
    
    f = phantom.fs.open(phantom.args[0], "w");
    if ( f ) {
        f.writeLine(content);
        f.close();
    }

    phantom.exit();
}