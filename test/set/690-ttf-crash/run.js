var page = require('webpage').create();
var url = 'http://localhost:5000';

page.open(url, function (status) {
    console.log('Page loaded once', status);
    page.release();
    page = require('webpage').create();
    page.open(url, function (status) {
        console.log('Page loaded twice', status);
        phantom.exit();
    });
});
