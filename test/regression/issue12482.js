// https://github.com/ariya/phantomjs/issues/12482
// regression caused by fix for
// https://github.com/ariya/phantomjs/issues/12431

var webpage = require('webpage');
var pages = [
    webpage.create(),
    webpage.create(),
    webpage.create()
];

var yet_to_load = pages.length;
function loadHook (status) {
    if (status !== "success")
        console.log("FAIL: status = " + status);
    if (--yet_to_load == 0) {
        pages[1].close();
        setTimeout(function(){
            phantom.exit(0);
            console.log("FAIL: should not get here");
        }, 50);
    }
}

for (var i = 0; i < pages.length; i++) {
    pages[i].onConsoleMessage = function(msg) { console.log(msg); };
    pages[i].open(
        "data:text/html,<script>setTimeout(function(){console.log("+
        "'FAIL: page "+i+" survived');},100)</script>", loadHook);
}
