CONFIG -= qt
SOURCES = libxml2.cpp
mac {
    QMAKE_CXXFLAGS += -iwithsysroot /usr/include/libxml2
    LIBS += -lxml2
} else {
    PKGCONFIG += libxml-2.0
    CONFIG += link_pkgconfig
}
