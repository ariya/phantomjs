SOURCES += $$PWD/qguistatemachine.cpp
!contains(DEFINES, QT_NO_STATEMACHINE_EVENTFILTER) {
    HEADERS += \
	   $$PWD/qkeyeventtransition.h \
	   $$PWD/qmouseeventtransition.h \
	   $$PWD/qbasickeyeventtransition_p.h \
	   $$PWD/qbasicmouseeventtransition_p.h
    SOURCES += \
	   $$PWD/qkeyeventtransition.cpp \
	   $$PWD/qmouseeventtransition.cpp \
	   $$PWD/qbasickeyeventtransition.cpp \
	   $$PWD/qbasicmouseeventtransition.cpp
}
