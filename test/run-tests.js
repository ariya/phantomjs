/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Load Jasmine and the HTML reporter
phantom.injectJs("./lib/jasmine.js");
phantom.injectJs("./lib/jasmine-console.js");
phantom.injectJs("./lib/chai.js");

var should = chai.should();

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

// Setting the "working directory" to the "/test" directory
var fs = require('fs');
fs.changeWorkingDirectory(phantom.libraryPath);

// Load specs
phantom.injectJs("./phantom-spec.js");
phantom.injectJs("./webpage-spec.js");
phantom.injectJs("./webserver-spec.js");
phantom.injectJs("./fs-spec-01.js"); //< Filesystem Specs 01 (Basic)
phantom.injectJs("./fs-spec-02.js"); //< Filesystem Specs 02 (Attributes)
phantom.injectJs("./fs-spec-03.js"); //< Filesystem Specs 03 (Paths)
phantom.injectJs("./fs-spec-04.js"); //< Filesystem Specs 04 (Tests)
phantom.injectJs("./system-spec.js");
phantom.injectJs("./webkit-spec.js");
require("./module_spec.js");
require("./require/require_spec.js");
require("./cjk-text-codecs.js");

// Launch tests
var jasmineEnv = jasmine.getEnv();

// Add a ConsoleReporter to 1) print with colors on the console 2) exit when finished
jasmineEnv.addReporter(new jasmine.ConsoleReporter(function(msg){
    // Print messages straight to the console
    console.log(msg.replace('\n', ''));
}, function(reporter){
    // On complete
    phantom.exit(reporter.results().failedCount);
}, true));

// Launch tests
jasmineEnv.updateInterval = 1000;
jasmineEnv.execute();
