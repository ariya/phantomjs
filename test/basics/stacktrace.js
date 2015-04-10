var assert = require('../assert');

var stack;
phantom.onError = function(message, s) { stack = s; };

var helperFile = "../fixtures/parse-error-helper.js";
phantom.injectJs(helperFile);

assert.equal(stack[0].file, helperFile);
assert.equal(stack[0].line, 2);
