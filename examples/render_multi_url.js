// Render Multiple URLs to file
// FIXME: For now it is fine with pure domain names: don't think it would work with paths and stuff like that

var system = require('system');

// Extend the Array Prototype with a 'foreach'
Array.prototype.forEach = function (action) {
    var i, len;
    for ( i = 0, len = this.length; i < len; ++i ) {
        action(i, this[i], len);
    }
};

/**
 * Render a given url to a given file
 * @param url URL to render
 * @param file File to render to
 * @param callback Callback function
 */
function renderUrlToFile(url, file, callback) {
    var page = require('webpage').create();
    page.viewportSize = { width: 800, height : 600 };
    page.settings.userAgent = "Phantom.js bot";

    page.open(url, function(status){
       if ( status !== "success") {
           console.log("Unable to render '"+url+"'");
       } else {
           page.render(file);
       }
       delete page;
       callback(url, file);
    });
}

// Read the passed args
var arrayOfUrls;
if ( system.args.length > 1 ) {
    arrayOfUrls = Array.prototype.slice.call(system.args, 1);
} else {
    // Default (no args passed)
    console.log("Usage: phantomjs render_multi_url.js [domain.name1, domain.name2, ...]");
    arrayOfUrls = [
      'www.google.com',
      'www.bbc.co.uk',
      'www.phantomjs.org'
    ];
}

// For each URL
arrayOfUrls.forEach(function(pos, url, total){
    var file_name = "./" + url + ".png";

    // Render to a file
    renderUrlToFile("http://"+url, file_name, function(url, file){
        console.log("Rendered '"+url+"' at '"+file+"'");
        if ( pos === total-1 ) {
            // Close Phantom if it's the last URL
            phantom.exit();
        }
    });
});
