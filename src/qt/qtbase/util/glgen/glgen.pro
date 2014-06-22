QT       -= gui
CONFIG   += console
CONFIG   -= app_bundle

# Uncomment following to enable debug output
#DEFINES += SPECPARSER_DEBUG

TEMPLATE  = app
TARGET    = glgen

SOURCES      += main.cpp \
                specparser.cpp \
                codegenerator.cpp

HEADERS      += \
                specparser.h \
                codegenerator.h

OTHER_FILES  += \
                qopenglversionfunctions.h.header \
                qopenglversionfunctions.h.footer \
                qopenglversionfunctions.cpp.header \
                qopenglversionfunctions.cpp.footer \
                qopenglversionfunctions__VERSION__.cpp.footer \
                qopenglversionfunctions__VERSION__.cpp.header \
                qopenglversionfunctions__VERSION__.h.footer \
                qopenglversionfunctions__VERSION__.h.header \
                qopenglversionfunctionsfactory_p.h.header \
                qopenglextensions.cpp.header \
                qopenglextensions.cpp.footer \
                qopenglextensions.h.header \
                qopenglextensions.h.footer \
                qopenglversionfunctionsfactory.cpp.header \
                qopenglversionfunctionsfactory.cpp.footer \
                README.txt
