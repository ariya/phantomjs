// List all the files in a Tree of Directories

if (phantom.args.length !== 1) {
    console.log("Usage: phantomjs scandir.js DIRECTORY_TO_SCAN");
    phantom.exit();
}

var scanDirectory = function (path) {
    if (phantom.fs.exists(path) && phantom.fs.isFile(path)) {
        console.log(path);
    } else if (phantom.fs.isDir(path)) {
        phantom.fs.list(path).forEach(function (e) {
            if ( e !== "." && e !== ".." ) {    //< Avoid loops
                scanDirectory(path + '/' + e);
            }
        });
    }
};
scanDirectory(phantom.args[0]);
phantom.exit();