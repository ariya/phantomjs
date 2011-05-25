var page = new WebPage();

page.open('http://www.httpuseragent.org', function (status) {
    if (status !== 'success') {
        console.log('Unable to access network');
    } else {
        var ua = page.evaluate(function () {
            return document.getElementById('myagent').innerText;
        });
        console.log(ua);
    }
    phantom.exit();
});
