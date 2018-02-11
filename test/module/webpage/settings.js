var assert = require('../../assert');
var webpage = require('webpage');

var page = webpage.create();
assert.typeOf(page, 'object');
assert.typeOf(page.settings, 'object');


// test default settings
var defaultPageSettings = JSON.parse(JSON.stringify(phantom.defaultPageSettings));
for (var p in defaultPageSettings) {
    assert.equal(page.settings[p], defaultPageSettings[p]); 
}

// test userAgent
var newAgent = 'test agent';
function getAcutalAgent() {
    return page.evaluate(function() { return navigator.userAgent; });
}
assert.equal(getAcutalAgent(), page.settings.userAgent);
page.settings.userAgent = newAgent;
assert.equal(getAcutalAgent(), newAgent);
page.openUrl('http://google.com', 'get', defaultPageSettings);
assert.equal(getAcutalAgent(), defaultPageSettings.userAgent);

/* todo: should also check those:
    * XSSAuditingEnabled
    * javascriptCanCloseWindows
    * javascriptCanOpenWindows
    * javascriptEnabled
    * loadImages
    * localToRemoteUrlAccessEnabled
    * webSecurityEnabled
*/