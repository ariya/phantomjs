if (phantom.storage.length === 0) {
    if (phantom.arguments.length !== 1) {
        phantom.log('Usage: run-jasmine.js URL');
        phantom.exit();
    } else {
        phantom.storage = 'run-jasmine';
        phantom.open(phantom.arguments[0]);
    }
} else {
    window.setInterval(function () {
        var list, el, desc, i, j;
        if (document.body.querySelector('.finished_at')) {
            phantom.log(document.body.querySelector('.description').innerText);
            list = document.body.querySelectorAll('div.jasmine_reporter > div.suite.failed');
            for (i = 0; i < list.length; ++i) {
                el = list[i];
                desc = el.querySelectorAll('.description');
                phantom.log('');
                for (j = 0; j < desc.length; ++j) {
                    phantom.log(desc[j].innerText);
                }
            }
            phantom.exit();
        }
    }, 100);
}
