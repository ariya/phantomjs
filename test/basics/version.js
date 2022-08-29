// This is separate from basics/phantom-object.js because it has to be
// updated with every release.
test(function () {
<<<<<<< HEAD
    assert_equals(phantom.version.major, 3);
    assert_equals(phantom.version.minor, 0);
=======
    assert_equals(phantom.version.major, 2);
    assert_equals(phantom.version.minor, 5);
>>>>>>> origin/wip
    assert_equals(phantom.version.patch, 0);
}, "PhantomJS version number is accurate");
