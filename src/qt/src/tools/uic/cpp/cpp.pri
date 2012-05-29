INCLUDEPATH += $$PWD $$QT_BUILD_TREE/src/tools/uic

DEFINES += QT_UIC_CPP_GENERATOR

# Input
HEADERS += $$PWD/cppextractimages.h \
           $$PWD/cppwritedeclaration.h \
           $$PWD/cppwriteicondata.h \
           $$PWD/cppwriteicondeclaration.h \
           $$PWD/cppwriteiconinitialization.h \
           $$PWD/cppwriteincludes.h \
           $$PWD/cppwriteinitialization.h

SOURCES += $$PWD/cppextractimages.cpp \
           $$PWD/cppwritedeclaration.cpp \
           $$PWD/cppwriteicondata.cpp \
           $$PWD/cppwriteicondeclaration.cpp \
           $$PWD/cppwriteiconinitialization.cpp \
           $$PWD/cppwriteincludes.cpp \
           $$PWD/cppwriteinitialization.cpp
