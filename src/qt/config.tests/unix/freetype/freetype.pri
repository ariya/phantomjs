!cross_compile {
    TRY_INCLUDEPATHS = /include /usr/include $$QMAKE_INCDIR $$QMAKE_INCDIR_X11 $$INCLUDEPATH
    # LSB doesn't allow using headers from /include or /usr/include
    linux-lsb-g++:TRY_INCLUDEPATHS = $$QMAKE_INCDIR $$QMAKE_INCDIR_X11 $$INCLUDEPATH
    for(p, TRY_INCLUDEPATHS) {
        p = $$join(p, "", "", "/freetype2")
        exists($$p):INCLUDEPATH *= $$p
    }
}
