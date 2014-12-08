var assert = require('../../assert');
var webpage = require('webpage');

var defaultPage = webpage.create();
assert.jsonEqual(defaultPage.scrollPosition, {left:0,top:0});

var options = {
    scrollPosition: {
        left: 1,
        top: 2
    }
};
var customPage = webpage.create(options);
assert.jsonEqual(customPage.scrollPosition, options.scrollPosition);
