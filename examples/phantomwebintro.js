// Read the Phantom webpage '.version' element text using jQuery and "includeJs"

"use strict";
var page = require('webpage').create();

page.onConsoleMessage = function(msg) {
    console.log(msg);
};

page.open("http://phantomjs.org/", function(status) {
    if (status === "success") {
        page.includeJs("http://ajax.googleapis.com/ajax/libs/jquery/3.1.0/jquery.min.js", function() {
            page.evaluate(function() {
                // lastest version on the web
                console.log("$(\"span.version\").text() -> " + $("span.version").text());
            });
            phantom.exit(0);
        });
    } else {
      phantom.exit(1);
    }
});
