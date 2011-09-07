(function (opts) {
    var i;

    opts = opts || {};

    if (typeof opts !== 'object') {
        return;
    }

    for (i in opts) {
        if (opts.hasOwnProperty(i) && typeof opts[i] !== 'undefined') {
            config[i] = opts[i];
        }
    }

	return null;
})((%1));
