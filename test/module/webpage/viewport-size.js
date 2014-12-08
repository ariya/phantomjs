var assert = require('../../assert');
var webpage = require('webpage');

var defaultPage = webpage.create();
assert.jsonEqual(defaultPage.viewportSize, {height:300,width:400});

var options = {
    viewportSize: {
        height: 100,
        width: 200
    }
};
var customPage = webpage.create(options);
assert.jsonEqual(customPage.viewportSize, options.viewportSize);
