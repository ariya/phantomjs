/**
 * This flag notifies inspector that it was deployed with the help
 * of a build system. Build system flattenes all css and js files,
 * so in this case inspector has to correct paths for dynamic resource loading.
 */
window.flattenImports = true;
window.DEBUG = false;