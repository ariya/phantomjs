// List all the files in a Tree of Directories

if (phantom.args.length !== 1) {
    console.log("Usage: phantomjs scandir.js DIRECTORY_TO_SCAN");
    phantom.exit();
}

var scanDirectory = function (path) {
    var fs = require('fs');
    if (fs.exists(path) && fs.isFile(path)) {
        console.log(path);
    } else if (fs.isDirectory(path)) {
        fs.list(path).forEach(function (e) {
            if ( e !== "." && e !== ".." ) {    //< Avoid loops
                scanDirectory(path + '/' + e);
            }
        });
    }
};
scanDirectory(phantom.args[0]);
phantom.exit();
