# Qt accessibility module

contains(QT_CONFIG, accessibility) {
      HEADERS += accessible/qaccessible.h \
                 accessible/qaccessible2.h \
		 accessible/qaccessibleobject.h \
		 accessible/qaccessiblewidget.h \
		 accessible/qaccessibleplugin.h
      SOURCES += accessible/qaccessible.cpp \
                 accessible/qaccessible2.cpp \
		 accessible/qaccessibleobject.cpp \
		 accessible/qaccessiblewidget.cpp \
		 accessible/qaccessibleplugin.cpp

      mac:!embedded:!qpa {
        HEADERS += accessible/qaccessible_mac_p.h
        OBJECTIVE_SOURCES += accessible/qaccessible_mac.mm \
                             accessible/qaccessible_mac_cocoa.mm
      } else:win32 { 
        SOURCES += accessible/qaccessible_win.cpp
      } else {
        HEADERS += accessible/qaccessiblebridge.h
        SOURCES += accessible/qaccessible_unix.cpp accessible/qaccessiblebridge.cpp
      }
}
