var assert = require('../../assert');

assert.typeOf(Function.length, 'number');
assert.typeOf(Function.prototype, 'function');
assert.typeOf(Function.prototype.apply, 'function');
assert.typeOf(Function.prototype.bind, 'function');
assert.typeOf(Function.prototype.call, 'function');
assert.typeOf(Function.prototype.name, 'string');
assert.typeOf(Function.prototype.toString, 'function');

var f = function foo(){};
assert.equal(f.name, 'foo');

assert.equal(Function.length, 1);
assert.equal(function(){}.length, 0);
assert.equal(function(x){}.length, 1);
assert.equal(function(x, y){}.length, 2);
assert.equal(function(x, y){}.length, 2);
