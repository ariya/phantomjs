var assert = require('../assert');

assert.typeOf(phantom, 'object');

assert.isTrue(phantom.hasOwnProperty('libraryPath'));
assert.typeOf(phantom.libraryPath, 'string');
assert.isTrue(phantom.libraryPath.length > 0);

assert.isTrue(phantom.hasOwnProperty('outputEncoding'));
assert.typeOf(phantom.outputEncoding, 'string');
assert.equal(phantom.outputEncoding.toLowerCase(), 'utf-8'); // default

assert.isTrue(phantom.hasOwnProperty('injectJs'));
assert.typeOf(phantom.injectJs, 'function');

assert.isTrue(phantom.hasOwnProperty('exit'));
assert.typeOf(phantom.exit, 'function');

assert.isTrue(phantom.hasOwnProperty('cookiesEnabled'));
assert.typeOf(phantom.cookiesEnabled, 'boolean');
assert.isTrue(phantom.cookiesEnabled);
