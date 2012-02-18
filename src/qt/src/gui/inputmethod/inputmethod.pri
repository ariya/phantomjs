# Qt inputmethod module

HEADERS +=inputmethod/qinputcontextfactory.h \
          inputmethod/qinputcontextplugin.h \
          inputmethod/qinputcontext_p.h \
          inputmethod/qinputcontext.h
SOURCES +=inputmethod/qinputcontextfactory.cpp \
          inputmethod/qinputcontextplugin.cpp \
          inputmethod/qinputcontext.cpp
x11 {
    HEADERS += inputmethod/qximinputcontext_p.h
    SOURCES += inputmethod/qximinputcontext_x11.cpp
}
win32 {
    HEADERS += inputmethod/qwininputcontext_p.h
    SOURCES += inputmethod/qwininputcontext_win.cpp
}
embedded {
    HEADERS += inputmethod/qwsinputcontext_p.h
    SOURCES += inputmethod/qwsinputcontext_qws.cpp
}
mac:!embedded:!qpa {
    HEADERS += inputmethod/qmacinputcontext_p.h
    SOURCES += inputmethod/qmacinputcontext_mac.cpp
}
symbian:contains(QT_CONFIG, s60) {
    HEADERS += inputmethod/qcoefepinputcontext_p.h
    SOURCES += inputmethod/qcoefepinputcontext_s60.cpp
    LIBS += -lfepbase -lakninputlanguage
}

