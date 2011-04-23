/**
 * Wait until the test condition is true or a timeout occurs. Useful for waiting
 * on a server response or for a ui change (fadeIn, etc.) to occur.
 *
 * @param testFx javascript condition that evaluates to a boolean,
 * it can be passed in as a string (e.g.: "1 == 1" or "$('#bar').is(':visible')" or
 * as a callback function.
 * @param message a message describing e.g. the ui change
 * @param timeOutMillis the max amount of time to wait. If not specified, 3 sec is used.
 */
function waitFor(testFx, message, timeOutMillis) {
    var maxtimeOutMillis = timeOutMillis ? timeOutMillis : 3000;
    var start = new Date().getTime();
    var condition = false;
    while(new Date().getTime() - start < maxtimeOutMillis) {
        phantom.sleep(250);
        condition = (typeof(testFx) == "string" ? eval(testFx) : testFx());
        if(condition) break;
    }
    if(!condition) {
        console.log("Timeout: " + message);
        phantom.exit(1)
    } else {
        console.log(message);
        console.log("+++ waitUntil finished in " + (new Date().getTime() - start) + " millis.");
    }
}

// example use:
if(!phantom.state) {
    // load a twitter page
    phantom.state = "loaded";
    phantom.open("http://twitter.com/#!/senchainc");
} else {
    // click the "sign in" link
    $(".signin-link").click();

    // wait for the sign in dialog to pop up
    waitFor(function() {
        return $("#signin-dropdown").is(":visible");
    }, "The sign-in dialog should be visible");
    phantom.exit();
}
