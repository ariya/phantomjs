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

var args, keys, str, enumerable;
(function() {
    args = arguments;
    keys = Object.keys(arguments);
    str = JSON.stringify(arguments);
    enumerable = false;
    for (var i in arguments) enumerable = true;
})(14);

assert.typeOf(args, 'object');
assert.typeOf(args.length, 'number');
assert.strictEqual(args.toString(), '[object Arguments]');
assert.strictEqual(args.length, 1);
assert.strictEqual(args[0], 14);

assert.typeOf(keys.length, 'number');
assert.strictEqual(keys.length, 1);
assert.equal(keys[0], 0);

assert.strictEqual(str, '{"0":14}');
assert.isTrue(enumerable);
