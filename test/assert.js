
// After the given timeout (in seconds), the script automatically terminates.
var timeout = 0.25;

var total = 0;

function resetExitTimer() {
    window.clearTimeout(window.autoExitTimer);
    window.autoExitTimer = window.setTimeout(function () {
        console.log(total, 'tests passed.');
        phantom.exit(0);
    }, timeout * 1000);
}

function showError() {
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

function showErrorEqual(actual, expected) {
    console.error('AssertionError: expected',  actual, 'to be', expected);

    showError();
}

function showErrorNotEqual(actual, expected) {
    console.error('AssertionError: expected',  actual,
        'to be different than', expected);

    showError();
}

function isTrue(value) {
    resetExitTimer();
    if (!value) {
        showErrorEqual(value, true);
    }
    ++total;
}

function equal(actual, expected) {
    resetExitTimer();
    if (actual != expected) {
        showErrorEqual(actual, expected);
    }
    ++total;
}

function jsonEqual(actual, expected) {
    resetExitTimer();
    var actualJson = JSON.stringify(actual);
    var expectedJson = JSON.stringify(expected);
    if (actualJson != expectedJson) {
        showErrorEqual(actualJson, expectedJson);
    }
    ++total;
}

function notEqual(actual, expected) {
    resetExitTimer();
    if (actual == expected) {
        showErrorNotEqual(actual, expected);
    }
    ++total;
}

function strictEqual(actual, expected) {
    resetExitTimer();
    if (actual !== expected) {
        showErrorEqual(actual, expected);
    }
    ++total;
}

function typeOf(object, expected) {
    resetExitTimer();
    if (typeof object !== expected) {
        showErrorEqual(typeof object, expected);
    }
    ++total;
}

exports.timeout = timeout;
exports.isTrue = isTrue;
exports.equal = equal;
exports.jsonEqual = jsonEqual;
exports.notEqual = notEqual;
exports.strictEqual = strictEqual;
exports.typeOf = typeOf;
