TEMPLATE = subdirs
CONFIG += ordered

include(WebKit.pri)

!v8 {
    exists($$PWD/JavaScriptCore/JavaScriptCore.pro): SUBDIRS += JavaScriptCore/JavaScriptCore.pro
    exists($$PWD/JavaScriptCore/jsc.pro): SUBDIRS += JavaScriptCore/jsc.pro
}

webkit2:exists($$PWD/WebKit2/WebKit2.pro): SUBDIRS += WebKit2/WebKit2.pro

SUBDIRS += WebCore
SUBDIRS += WebKit/qt/QtWebKit.pro

webkit2 {
    exists($$PWD/WebKit2/WebProcess.pro): SUBDIRS += WebKit2/WebProcess.pro
    exists($$PWD/WebKit2/UIProcess/API/qt/tests): SUBDIRS += WebKit2/UIProcess/API/qt/tests
}

contains(QT_CONFIG, declarative) {
    exists($$PWD/WebKit/qt/declarative): SUBDIRS += WebKit/qt/declarative
}

exists($$PWD/WebKit/qt/tests): SUBDIRS += WebKit/qt/tests

build-qtscript {
    SUBDIRS += \
        JavaScriptCore/qt/api/QtScript.pro \
        JavaScriptCore/qt/tests \
        JavaScriptCore/qt/benchmarks
}

symbian {
    exists($$PWD/WebKit/qt/symbian/platformplugin): SUBDIRS += WebKit/qt/symbian/platformplugin

    # Forward the install target to WebCore. A workaround since INSTALLS is not implemented for symbian
    install.commands = $(MAKE) -C WebCore install
    QMAKE_EXTRA_TARGETS += install
}

include(WebKit/qt/docs/docs.pri)
