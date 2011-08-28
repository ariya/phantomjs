// Load Jasmine and the HTML reporter
phantom.injectJs("./lib/jasmine.js");
phantom.injectJs("./lib/jasmine-console.js");

// Helper funcs
function expectHasFunction(o, name) {
    it("should have '" + name + "' function", function() {
        expect(typeof o[name]).toEqual('function');
    });
}

function expectHasProperty(o, name) {
    it("should have '" + name + "' property", function() {
        expect(o.hasOwnProperty(name)).toBeTruthy();
    });
}

function expectHasPropertyString(o, name) {
    expectHasProperty(o, name);

    it("should have '" + name + "' as a string", function() {
        expect(typeof o[name]).toEqual('string');
    });
}

// Load specs
phantom.injectJs("./phantom-spec.js");
phantom.injectJs("./webpage-spec.js");
phantom.injectJs("./fs-spec-01.js"); //< Filesystem Specs 01 (Basic)
phantom.injectJs("./fs-spec-02.js"); //< Filesystem Specs 02 (Attributes)
phantom.injectJs("./fs-spec-03.js"); //< Filesystem Specs 03 (Paths)
phantom.injectJs("./fs-spec-04.js"); //< Filesystem Specs 04 (Tests)

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
