SOURCES = iwmmxt.cpp
CONFIG -= x11 qt
isEmpty(QMAKE_CFLAGS_IWMMXT):error("This compiler does not support iWMMXt")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_IWMMXT
