/* Test the test server itself. */

var assert = require('../assert');
var webpage = require('webpage');
var page = webpage.create();

var urlsToTest = [
    'http://localhost:9180/hello.html',
    'http://localhost:9180/status?200',
    'http://localhost:9180/echo'
];
var i = 0;

page.onResourceReceived = function (response) {
    assert.equal(response.status, 200);
};

page.onLoadFinished = function (status) {
    assert.equal(status, 'success');
    i++;
    if (i == urlsToTest.length) {
        phantom.exit(0);
    } else {
        page.open(urlsToTest[i]);
    }
}

page.open(urlsToTest[i]);
