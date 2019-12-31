test(function () {
    var result = [2, 3, 5].map(x => x * x);
    assert_equals(result.length, 3);
    assert_equals(result.join(' '), '4 9 25');
}, "ES2015 arrow function");
