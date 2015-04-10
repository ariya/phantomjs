!cross_compile {
    TRY_INCLUDEPATHS = /include /usr/include $$QMAKE_INCDIR $$QMAKE_INCDIR_X11 $$INCLUDEPATH
    # LSB doesn't allow using headers from /include or /usr/include
    linux-lsb-g++:TRY_INCLUDEPATHS = $$QMAKE_INCDIR $$QMAKE_INCDIR_X11 $$INCLUDEPATH
    for(p, TRY_INCLUDEPATHS) {
        p = $$join(p, "", "", "/freetype2")
        exists($$p):INCLUDEPATH *= $$p
    }
} else {
   # If we are cross-compiling, then there is still a remote possibility that
   # configure detected font-config & freetype,  stored in these variables.
   QMAKE_CFLAGS += $$QMAKE_CFLAGS_FONTCONFIG
   QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_FONTCONFIG
}
