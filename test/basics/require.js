// Windows needs absolute paths
var rtests = require(phantom.libraryPath + "/require/require_spec.js").tests;

for (var i = 0; i < rtests.length; i++) {
    test.apply(null, rtests[i]);
}
