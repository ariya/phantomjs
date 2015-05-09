CONFIG -= qt
SOURCES = libxslt.cpp
mac {
    QMAKE_CXXFLAGS += -iwithsysroot /usr/include/libxslt -iwithsysroot /usr/include/libxml2
    LIBS += -lxslt
} else {
    PKGCONFIG += libxslt
    CONFIG += link_pkgconfig
}
