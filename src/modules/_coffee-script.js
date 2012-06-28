var fs = require('fs');

require.stub('vm');
require.stub('path');
require.stub('fs', {
    readFileSync: function(path, encoding) {
        return fs.read(path);
    }
});

module.exports = require('../coffee-script');
