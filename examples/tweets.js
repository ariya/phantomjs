// Get twitter status for given account (or for the default one, "HeadlessPhantom")

var page = require('webpage').create(),
    system = require('system'),
    twitterId = "HeadlessPhantom"; //< default value

// Route "console.log()" calls from within the Page context to the main Phantom context (i.e. current "this")
page.onConsoleMessage = function(msg) {
    console.log(msg);
};

// Print usage message, if no twitter ID is passed
if (system.args.length < 2) {
    console.log("Usage: tweets.js [twitter ID]");
} else {
    twitterId = system.args[1];
}

// Heading
console.log("*** Latest tweets from @" + twitterId + " ***\n");

// Open Twitter Mobile and, onPageLoad, do...
page.open(encodeURI("http://mobile.twitter.com/" + twitterId), function (status) {
    // Check for page load success
    if (status !== "success") {
        console.log("Unable to access network");
    } else {
        // Execute some DOM inspection within the page context
        page.evaluate(function() {
            var list = document.querySelectorAll('span.status');
            for (var i = 0; i < list.length; ++i) {
                console.log((i + 1) + ": " + list[i].innerHTML.replace(/<.*?>/g, ''));
            }
        });
    }
    phantom.exit();
});
