// Load Jasmine and the HTML reporter
phantom.injectJs("./lib/jasmine.js");
phantom.injectJs("./lib/jasmine-console.js");

// Load specs
phantom.injectJs("./phantom-spec.js");
phantom.injectJs("./fs-spec-01.js"); //< Filesystem Specs 01 (Basic)
phantom.injectJs("./fs-spec-02.js"); //< Filesystem Specs 02 (Attributes)

// Launch tests
var jasmineEnv = jasmine.getEnv();

// Add a ConsoleReporter to 1) print with colors on the console 2) exit when finished
jasmineEnv.addReporter(new jasmine.ConsoleReporter(function(msg){
    // Print messages straight to the console
    console.log(msg.replace('\n', ''));
}, function(reporter){
    // On complete
    phantom.exit();
}, true));

// Launch tests
jasmineEnv.updateInterval = 1000;
jasmineEnv.execute();
