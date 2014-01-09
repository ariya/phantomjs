/*
 * CommonJS System/1.0
 * Spec: http://wiki.commonjs.org/wiki/System/1.0
 */

exports.create = function () {
	var systemLibs = phantom.createSystemLibs();
	return systemLibs;
}
