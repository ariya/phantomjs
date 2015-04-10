var assert = require('../../assert');
var webpage = require('webpage');

var defaultPage = webpage.create();
assert.jsonEqual(defaultPage.clipRect, {height:0,left:0,top:0,width:0});

var options = {
    clipRect: {
        height: 100,
        left: 10,
        top: 20,
        width: 200
    }
};
var customPage = new WebPage(options);
assert.jsonEqual(customPage.clipRect, options.clipRect);
