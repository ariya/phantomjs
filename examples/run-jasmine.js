var logSpecFailures = function(specs) {
    for (var i = 0; i < specs.length; i++) {
        var spec = specs[i];

        var results = spec.results();
        if (!results.passed()) {
            console.log("");
            console.log(spec.suite.getFullName() + " " + spec.description + " failed:");

            var items = results.getItems();
            for (var j = 0; j < items.length; j++) {
                var result = items[j];
                if (result.passed && !result.passed()) {
                  console.log(result.toString());
                }
            }
        }
    }
};

if (phantom.state.length === 0) {
    if (phantom.args.length < 1) {
        console.log("Usage: phantomjs.exe run-jasmine.js path/to/spec/runner/html/file [output file]");
        phantom.exit(1);
    } else {
        phantom.state = "run-jasmine";
        phantom.openFile(phantom.args[0]);
    }
} else {
    var file = phantom.args[1];
    window.fileWriter = {
        write: function(content) {
            if (file) {
                phantom.writeToFile(file, content);
            }
        }
    };

    window.setInterval(function () {
        var status = document.body.querySelector(".runner .description");
        if (status && window.jasmine) {
            var runner = window.jasmine.getEnv().currentRunner();

            var failures = runner.results().failedCount;
            if (failures) {
                logSpecFailures(runner.specs());
            }

            console.log("");
            console.log(status.innerText);

            phantom.exit(failures ? 1 : 0);
        }
    }, 100);
}
