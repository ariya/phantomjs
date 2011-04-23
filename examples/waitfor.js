/**
 * Wait until the test condition is true or a timeout occurs. Useful for waiting
 * on a server response or for a ui change (fadeIn, etc.) to occur.
 *
 * @param testFx javascript condition that evaluates to a boolean,
 * it can be passed in as a string (e.g.: "1 == 1" or "$('#bar').is(':visible')" or
 * as a callback function.
 * @param message a message to show on failure
 * @param timeOutMillis the max amount of time to wait. If not specified, 30sec-s is used.
 */
function waitFor(testFx, message, timeOutMillis) {
    var maxtimeOutMillis = timeOutMillis ? timeOutMillis : 30000;
    var start = new Date().getTime();
    var condition = false;
    while(new Date().getTime() - start < maxtimeOutMillis) {
        phantom.sleep(250);
        condition = (typeof(testFx) == "string" ? eval(testFx) : testFx());
        if(condition) break;
    }
    if(!condition) {
        throw Error("Timeout: " + message);
    }
    console.log("+++ waitUntil finished in " + (new Date().getTime() - start) + " millis.");
}

// example use:
if(!phantom.state) {
    // load a twitter page
    phantom.state = "loaded";
    phantom.open("http://twitter.com/WiIlFerreII");
} else {
    // click the "sign in" link
    $(".signin").click();

    // wait for the sign in dialog to pop up
    waitFor(function() {
        return $("#signin_menu").is(":visible");
    }, "The sign-in dialog should be visible");
    phantom.exit();
}
