test(function () {
    var x = 14, y = 3;
    var obj = { x, y };
    assert_type_of(obj.x, 'number');
    assert_type_of(obj.y, 'number');
    assert_equals(obj.x, 14);
    assert_equals(obj.y, 3);
}, "ES2015 object literal property shorthand");
