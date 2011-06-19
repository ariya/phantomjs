/**
 * Collection of Core JavaScript utility functionalities.
 */

// Namespace "utils.core"
var utils = utils || {};
utils.core = utils.core || {};

/**
 * Wait until the test condition is true or a timeout occurs. Useful for waiting
 * on a server response or for a ui change (fadeIn, etc.) to occur.
 *
 * @param check javascript condition that evaluates to a boolean.
 * @param onTestPass what to do when 'check' condition is fulfilled.
 * @param onTimeout what to do when 'check' condition is not fulfilled and 'timeoutMs' has passed
 * @param timeoutMs the max amount of time to wait. Default value is 3 seconds
 * @param freqMs how frequently to repeat 'check'. Default value is 250 milliseconds
 */
utils.core.waitfor = function(check, onTestPass, onTimeout, timeoutMs, freqMs) {
    var timeoutMs = timeoutMs || 3000,      //< Default Timeout is 3s
        freqMs = freqMs || 250,             //< Default Freq is 250ms
        start = Date.now(),
        condition = false,
        timer = setTimeout(function() {
            var elapsedMs = Date.now() - start;
            if ((elapsedMs - start < timeoutMs) && !condition) {
                // If not time-out yet and condition not yet fulfilled
                condition = check(elapsedMs);
                timer = setTimeout(arguments.callee, freqMs);
            } else {
                clearTimeout(timer); //< house keeping
                if (!condition) {
                    // If condition still not fulfilled (timeout but condition is 'false')
                    onTimeout(elapsedMs);
                } else {
                    // Condition fulfilled (timeout and/or condition is 'true')
                    onTestPass(elapsedMs);
                }
            }
        }, freqMs);
};