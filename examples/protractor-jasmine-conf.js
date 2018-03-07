var HtmlReporter = require('protractor-jasmine2-screenshot-reporter');

var reporter = new HtmlReporter({
  dest: 'protractor-reports',
  filename: 'protractor-report.html'
});

// An example configuration file.
exports.config = {
  directConnect: true,

  // Capabilities to be passed to the webdriver instance.
  capabilities: {
    'browserName': 'chrome',
    //'phantomjs.binary.path': './node_modules/phantomjs/bin/phantomjs'
  },

  // Framework to use. Jasmine is recommended.
  framework: 'jasmine2',

  // Spec patterns are relative to the current working directory when
  // protractor is called.
  specs: ['checklist/checklist-spec.js'],

  // Options to be passed to Jasmine.
  jasmineNodeOpts: {
    defaultTimeoutInterval: 170000
  },

    //Before launch function to run initial configurations before start running the test
    beforeLaunch: function (){
        return new Promise(function(resolve){
            reporter.beforeLaunch(resolve);
        });
    },

  onPrepare: function() {
   jasmine.getEnv().addReporter(reporter);
 },

 allScriptsTimeout: 500000
};
