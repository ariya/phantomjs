var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

var message;
page.onConsoleMessage = function (msg) {
    message = msg;
}

// console.log should support multiple arguments
page.evaluate(function () {
    console.log('answer', 42);
});

assert.equal(message, 'answer 42');
