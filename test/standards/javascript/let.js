test(function () {
    let x = 4;
    if (true) {
	let x = 7;
	assert_equals(x, 7);
    }
    assert_equals(x, 4);
}, "ES2015 block-scope let");
