#! [quoting library paths with spaces]
win32:LIBS += "C:/mylibs/extra libs/extra.lib"
unix:LIBS += "-L/home/user/extra libs" -lextra
#! [quoting library paths with spaces]

#! [quoting include paths with spaces]
win32:INCLUDEPATH += "C:/mylibs/extra headers"
unix:INCLUDEPATH += "/home/user/extra headers"
#! [quoting include paths with spaces]
