test(function () {
    assert_type_of(Array.isArray, 'function');
    assert_is_true(Array.isArray([]));
    assert_is_true(Array.isArray([1]));
    assert_is_true(Array.isArray([1, 2]));
    assert_is_false(Array.isArray({}));
    assert_is_false(Array.isArray(null));
}, "ES2015 Array.isArray");
