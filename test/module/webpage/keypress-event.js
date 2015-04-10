var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

page.evaluate(function() {
    window.addEventListener('keypress', function(event) {
        window.loggedEvent = window.loggedEvent || [];
        window.loggedEvent.push(event);
    }, false);
});

page.sendEvent('keypress', page.event.key.C);
var loggedEvent = page.evaluate(function() {
    return window.loggedEvent;
});

assert.equal(loggedEvent.length, 1);
assert.equal(loggedEvent[0].which, page.event.key.C);


// Send keypress events to an input element and observe the effect.

page.content = '<input type="text">';
page.evaluate(function() {
    document.querySelector('input').focus();
});

function getText() {
    return page.evaluate(function() {
        return document.querySelector('input').value;
    });
}

page.sendEvent('keypress', page.event.key.A);
assert.equal(getText(), 'A');
page.sendEvent('keypress', page.event.key.B);
assert.equal(getText(), 'AB');
page.sendEvent('keypress', page.event.key.Backspace);
assert.equal(getText(), 'A');
page.sendEvent('keypress', page.event.key.Backspace);
assert.equal(getText(), '');

page.sendEvent('keypress', 'XYZ');
assert.equal(getText(), 'XYZ');

// Special character: A with umlaut
page.sendEvent('keypress', 'ä');
assert.equal(getText(), 'XYZä');

// 0x02000000 is the Shift modifier.
page.sendEvent('keypress', page.event.key.Home, null, null,  0x02000000);
page.sendEvent('keypress', page.event.key.Delete);
assert.equal(getText(), '');

// Cut and Paste
// 0x04000000 is the Control modifier.
page.sendEvent('keypress', 'ABCD');
assert.equal(getText(), 'ABCD');
page.sendEvent('keypress', page.event.key.Home, null, null,  0x02000000);
page.sendEvent('keypress', 'x', null, null, 0x04000000);
assert.equal(getText(), '');
page.sendEvent('keypress', 'v', null, null, 0x04000000);
assert.equal(getText(), 'ABCD');
