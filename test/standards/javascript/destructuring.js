test(function () {
    var o = { d: 14, m: 3 };
    var { d: foo, m: bar } = o;
    assert_type_of(foo, 'number');
    assert_type_of(bar, 'number');
    assert_equals(foo, 14);
    assert_equals(bar, 3);
}, "ES2015 object destructuring");

test(function () {
    var d, m;
    [d, m] = [14, 3];
    assert_type_of(d, 'number');
    assert_type_of(m, 'number');
    assert_equals(d, 14);
    assert_equals(m, 3);
}, "ES2015 array destructuring");

test(function () {
    var x = 14, y = 3;
    [x, y] = [y, x];
    assert_equals(x, 3);
    assert_equals(y, 14);
}, "ES2015 variable swap");
