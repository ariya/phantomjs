// This is separate from basics/phantom-object.js because it has to be
// updated with every release.
test(function () {
    assert_equals(phantom.version.major, 2);
    assert_equals(phantom.version.minor, 1);
    assert_equals(phantom.version.patch, 1);
}, "PhantomJS version number is accurate");
