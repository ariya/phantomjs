// This is separate from basics/phantom-object.js because it has to be
// updated with every release.
test(function () {
    assert_equals(phantom.version.major, 2);
    assert_equals(phantom.version.minor, 2);
    assert_equals(phantom.version.patch, 0);
}, "PhantomJS version number is accurate");
