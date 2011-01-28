if (phantom.state.length === 0) {
    if (phantom.args.length === 0 || phantom.args.length > 2) {
        console.log('Usage: run-qunit.js URL');
        phantom.exit();
    } else {
        phantom.state = 'run-qunit';
        phantom.open(phantom.args[0]);
    }
} else {
    setInterval(function() {
        var el = document.getElementById('qunit-testresult');
        if (phantom.state !== 'finish') {
            if (el && el.innerText.match('completed')) {
                phantom.state = 'finish';
                console.log(el.innerText);
                try {
                    failed = el.getElementsByClassName('failed')[0].innerHTML;
                } catch (e) {
                }
                phantom.exit((parseInt(failed, 10) > 0) ? 1 : 0);
            }
        }
    }, 100);
}
