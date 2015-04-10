var assert = require('../../assert');
var page = require('webpage').create();

page.evaluate(function() {
    window.addEventListener('mousemove', function(event) {
        window.loggedEvent = window.loggedEvent || [];
        window.loggedEvent.push(event);
    }, false);
});

page.sendEvent('mousemove', 14, 3);
var loggedEvent = page.evaluate(function() {
    return window.loggedEvent;
});
assert.equal(loggedEvent.length, 1);
assert.equal(loggedEvent[0].clientX, 14);
assert.equal(loggedEvent[0].clientY, 3);
