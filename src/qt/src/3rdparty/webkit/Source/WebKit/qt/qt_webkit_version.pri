QT_WEBKIT_VERSION = 4.9.3
QT_WEBKIT_MAJOR_VERSION = 4
QT_WEBKIT_MINOR_VERSION = 9
QT_WEBKIT_PATCH_VERSION = 3

QT.webkit.name = QtWebKit
QT.webkit.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtWebKit
QT.webkit.sources = $$QT_MODULE_BASE
QT.webkit.libs = $$QT_MODULE_LIB_BASE
QT.webkit.depends = core gui opengl network xmlpatterns script

!contains(QT_CONFIG, modular)|contains(QT_ELIGIBLE_MODULES, webkit) {
    QT_CONFIG += webkit
} else {
    warning("Attempted to include $$QT.webkit.name in the build, but it was not enabled in configure.")
}
