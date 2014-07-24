(function () {
    var a = new Array(100000);
    for (var i = 0; i < 100000; ++i)
        a[i] = {};

    for (var i = 0; i < 500; ++i) {
        for (var j = 0; j < 100000; ++j)
            var b = {};
    }
})();
