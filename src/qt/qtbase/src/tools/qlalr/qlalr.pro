option(host_build)

SOURCES += \
    compress.cpp \
    cppgenerator.cpp \
    dotgraph.cpp \
    lalr.cpp \
    main.cpp \
    parsetable.cpp \
    recognizer.cpp \
    grammar.cpp

HEADERS += \
    compress.h \
    cppgenerator.h \
    dotgraph.h \
    lalr.h \
    parsetable.h \
    recognizer.h \
    grammar_p.h

OTHER_FILES += \
    lalr.g

load(qt_tool)
