/**
 * Wait until the test condition is true or a timeout occurs. Useful for waiting
 * on a server response or for a ui change (fadeIn, etc.) to occur. 
 * Code to execute on timeout can also be specified 
 *
 * @param testFx javascript condition that evaluates to a boolean,
 * it can be passed in as a string (e.g.: "1 == 1" or "$('#bar').is(':visible')" or
 * as a callback function.
 * @param onReady what to do when testFx condition is fulfilled,
 * it can be passed in as a string (e.g.: "1 == 1" or "$('#bar').is(':visible')" or
 * as a callback function.
 * @param onTimeOut what to do when its timeout
 * @param timeOutMillis the max amount of time to wait. If not specified, 3 sec is used.
 */
function waitFor(testFx, onReady, onTimeOut, timeOutMillis) {
    var maxtimeOutMillis = timeOutMillis ? timeOutMillis : 3000, //< Default Max Timout is 3s
        start = new Date().getTime(),
        condition = false,
        interval = setInterval(function() {
            if ( (new Date().getTime() - start < maxtimeOutMillis) && !condition ) {
                // If not time-out yet and condition not yet fulfilled
                condition = (typeof(testFx) === "string" ? eval(testFx) : testFx()); //< defensive code
            } else {
                clearInterval(interval); //< Stop this interval                
                if(!condition) {
                    // If condition still not fulfilled (timeout but condition is 'false')
                    //Check if the onTimeOut exists
                    if(onTimeOut) {
                        console.log("'waitFor()' timeout but onTimeOut function specified. Executing onTimeOut");
                        typeof(onTimeOut) === "string" ? eval(onTimeOut) : onTimeOut();
                    } else {
                        console.log("'waitFor()' timeout and nothing specified on timeout. Exiting...");
                        phantom.exit(1);
                    }    
                } else {
                    // Condition fulfilled (timeout and/or condition is 'true')
                    console.log("'waitFor()' finished in " + (new Date().getTime() - start) + "ms.");
                    typeof(onReady) === "string" ? eval(onReady) : onReady(); //< Do what it's supposed to do once the condition is fulfilled
                }
            }
        }, 250); //< repeat check every 250ms
};


var page = require('webpage').create();

// Open Twitter on 'sencha' profile and, onPageLoad, do...
page.open("http://twitter.com/#!/sencha", function (status) {
    // Check for page load success
    if (status !== "success") {
        console.log("Unable to access network");
    } else {
        // Wait for 'signin-dropdown' to be visible
        waitFor(function() {
            // Check in the page if a specific element is now visible
            return page.evaluate(function() {
                return $("#signin-dropdown").is(":visible");
            });
        }, function() {
            console.log("The sign-in dialog should be visible now.");
            phantom.exit();
        }, function() {
            console.log("The code to execute if timeout occured");
            phantom.exit();
        }, 5000);        
    }
});

