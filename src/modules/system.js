/*
 * CommonJS System/1.0
 * Spec: http://wiki.commonjs.org/wiki/System/1.0
 */

exports.platform = 'phantomjs';

Object.defineProperty(exports, 'stdout', {
    enumerable: true,
    writeable: false,
    get: function() {
        return exports.standardout;
    }
});

Object.defineProperty(exports, 'stdin', {
    enumerable: true,
    writeable: false,
    get: function() {
        return exports.standardin;
    }
});

Object.defineProperty(exports, 'stderr', {
    enumerable: true,
    writeable: false,
    get: function() {
        return exports.standarderr;
    }
});
