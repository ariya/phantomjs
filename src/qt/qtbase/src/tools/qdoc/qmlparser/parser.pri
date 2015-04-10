HEADERS += \
    $$PWD/qqmljsast_p.h \
    $$PWD/qqmljsastfwd_p.h \
    $$PWD/qqmljsastvisitor_p.h \
    $$PWD/qqmljsengine_p.h \
    $$PWD/qqmljsgrammar_p.h \
    $$PWD/qqmljslexer_p.h \
    $$PWD/qqmljsmemorypool_p.h \
    $$PWD/qqmljsparser_p.h \
    $$PWD/qqmljsglobal_p.h \
    $$PWD/qqmljskeywords_p.h \

SOURCES += \
    $$PWD/qqmljsast.cpp \
    $$PWD/qqmljsastvisitor.cpp \
    $$PWD/qqmljsengine_p.cpp \
    $$PWD/qqmljsgrammar.cpp \
    $$PWD/qqmljslexer.cpp \
    $$PWD/qqmljsparser.cpp \

OTHER_FILES += \
    $$PWD/qqmljs.g
