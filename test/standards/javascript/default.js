test(function () {
    function inc(x, step = 1) { return x + step }
    assert_equals(inc(4), 5);
    assert_equals(inc(4, 3), 7);
}, "ES2015 default parameter value");
