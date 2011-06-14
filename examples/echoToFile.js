// echoToFile.js - Write in a given file all the parameters passed on the CLI

if (phantom.args.length < 2) {
    console.log("Usage: echoToFile.js DESTINATION_FILE <arguments to echo...>");
    phantom.exit();
} else {
    var content = '';
    for ( i= 1; i < phantom.args.length; ++i ) {
        content += ' '+phantom.args[i];
    }
    
    phantom.saveToFile(phantom.args[0], content);
    phantom.exit();
}