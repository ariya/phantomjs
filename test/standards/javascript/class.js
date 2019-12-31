test(function () {
    class Vehicle { };
    assert_type_of(Vehicle, 'function');
}, "ES2015 class declaration");

test(function () {
    class Vehicle { constructor(n) { this.name = n } };
    var v = new Vehicle('daily driver');
    assert_type_of(Vehicle, 'function');
    assert_type_of(Vehicle.constructor, 'function');
    assert_type_of(v, 'object')
}, "ES2015 class constructor");


test(function () {
    class Vehicle {
	constructor(n) { this._name = n }
	get name() { return this._name }
    }
    var v = new Vehicle('daily driver');
    assert_type_of(v.name, 'string')
    assert_equals(v.name, 'daily driver');
}, "ES2015 class getter");

test(function () {
    class Vehicle {
	get name() { return this._name }
	set name(n) { this._name = n }
    }
    var v = new Vehicle();
    v.name = 'daily driver';
    assert_type_of(v.name, 'string')
    assert_equals(v.name, 'daily driver');
}, "ES2015 class setter");
