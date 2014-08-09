
// After the given timeout (in seconds), the script automatically terminates.
var timeout = 2;

var total = 0;

function resetExitTimer() {
    window.clearTimeout(window.autoExitTimer);
    window.autoExitTimer = window.setTimeout(function () {
        console.log(total, 'tests passed.');
        phantom.exit(0);
    }, timeout * 1000);
}

function showError(actual, expected) {
    console.error('AssertionError: expected',  actual, 'to be', expected);

    // Get the stack trace by throwing an exception and catching it.
    try {
        throw new Error();
    } catch (e) {
        var traces = e.stack.split('\n');
        // The first one is this showError() function, not useful.
        // The second is the assertion function, also superfluous.
        traces = traces.slice(3);
        console.log(traces.join('\n'));
    } finally {
        phantom.exit(1);
    }
}

function isTrue(value) {
    resetExitTimer();
    if (!value) {
        showError(value, true);
    }
    ++total;
}

function equal(actual, expected) {
    resetExitTimer();
    if (actual != expected) {
        showError(actual, expected);
    }
    ++total;
}

function strictEqual(actual, expected) {
    resetExitTimer();
    if (actual !== expected) {
        showError(actual, expected);
    }
    ++total;
}

function typeOf(object, expected) {
    resetExitTimer();
    if (typeof object !== expected) {
        showError(typeof object, expected);
    }
    ++total;
}

exports.timeout = timeout;
exports.isTrue = isTrue;
exports.equal = equal;
exports.strictEqual = strictEqual;
exports.typeOf = typeOf;
