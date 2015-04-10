var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();
assert.strictEqual(page.zoomFactor, 1.0);

page.zoomFactor = 1.5;
assert.strictEqual(page.zoomFactor, 1.5);

page.zoomFactor = 2.0;
assert.strictEqual(page.zoomFactor, 2.0);

page.zoomFactor = 0.5;
assert.strictEqual(page.zoomFactor, 0.5);

// TODO: render using zoomFactor != 1 and check the result
