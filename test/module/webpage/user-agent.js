var assert = require('../../assert');
var webpage = require('webpage');


var ua = 'PHANTOMJS-TEST-USER-AGENT';
var page = webpage.create({
    settings: {
        userAgent: ua
    }
});

assert.equal(page.settings.userAgent, ua);

page.open('http://localhost:9180/user-agent.html', function (status) {
    assert.equal(status, 'success');
    var agent = page.evaluate(function() {
        return document.getElementById('ua').textContent;
    });
    assert.equal(agent, ua);
});
