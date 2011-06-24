
phantom.injectJs("../utils/core.js");

if (phantom.args.length === 0 || phantom.args.length > 2) {
    console.log('Usage: run-jasmine.js URL');
    phantom.exit();
}

var page = new WebPage();

// Route "console.log()" calls from within the Page context to the main Phantom context (i.e. current "this")
page.onConsoleMessage = function(msg) {
    console.log(msg);
};

page.open(phantom.args[0], function(status){
    if (status !== "success") {
        console.log("Unable to access network");
        phantom.exit();
    } else {
        utils.core.waitfor(function(){
            return page.evaluate(function(){
                if (document.body.querySelector('.finished-at')) {
                    return true;
                }
                return false;
            });
        }, function(){
            page.evaluate(function(){
                console.log(document.body.querySelector('.description').innerText);
                list = document.body.querySelectorAll('div.jasmine_reporter > div.suite.failed');
                for (i = 0; i < list.length; ++i) {
                    el = list[i];
                    desc = el.querySelectorAll('.description');
                    console.log('');
                    for (j = 0; j < desc.length; ++j) {
                        console.log(desc[j].innerText);
                    }
                }
            });
            phantom.exit();
        });
    }
});
