var assert = require('../../assert');

assert.isTrue(window.hasOwnProperty('WebPage'));
assert.typeOf(window.WebPage, 'function');
