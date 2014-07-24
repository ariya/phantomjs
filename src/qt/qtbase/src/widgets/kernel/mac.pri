!x11:mac:!ios {
   LIBS_PRIVATE += -framework Carbon -framework Cocoa -lz
   *-mwerks:INCLUDEPATH += compat
}
