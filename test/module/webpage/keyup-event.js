var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

page.evaluate(function() {
    window.addEventListener('keyup', function(event) {
        window.loggedEvent = window.loggedEvent || [];
        window.loggedEvent.push(event);
    }, false);
});

page.sendEvent('keyup', page.event.key.B);
var loggedEvent = page.evaluate(function() {
    return window.loggedEvent;
});

assert.equal(loggedEvent.length, 1);
assert.equal(loggedEvent[0].which, page.event.key.B);
