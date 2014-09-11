var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();

// It should still fire `onResourceReceived` callback twice
// (for each start and end stage) when the resource error occured

var startStage = 0;
var endStage = 0;
page.onResourceReceived = function (resource) {
    if (resource.stage === 'start') {
        ++startStage;
    }
    if (resource.stage === 'end') {
        ++endStage;
    }
};

page.open('http://localhost:9180/status?400', function (status) {
    assert.equal(startStage, 1);
    assert.equal(endStage, 1);
});
