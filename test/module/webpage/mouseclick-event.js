var assert = require('../../assert');
var page = require('webpage').create();

page.evaluate(function() {
    window.addEventListener('mousedown', function(event) {
        window.loggedEvent = window.loggedEvent || {};
        window.loggedEvent.mousedown = event;
    }, false);
    window.addEventListener('mouseup', function(event) {
        window.loggedEvent = window.loggedEvent || {};
        window.loggedEvent.mouseup = event;
    }, false);
});
page.sendEvent('click', 42, 217);

var event = page.evaluate(function() {
    return window.loggedEvent;
});
assert.equal(event.mouseup.clientX, 42);
assert.equal(event.mouseup.clientY, 217);
assert.equal(event.mousedown.clientX, 42);
assert.equal(event.mousedown.clientY, 217);

// click with modifier key
page.evaluate(function() {
    window.addEventListener('click', function(event) {
        window.loggedEvent = window.loggedEvent || {};
        window.loggedEvent.click = event;
    }, false);
});
page.sendEvent('click', 100, 100, 'left', page.event.modifier.shift);

var event = page.evaluate(function() {
    return window.loggedEvent.click;
});
assert.isTrue(event.shiftKey);
