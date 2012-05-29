HEADERS += $$PWD/qstatemachine.h \
	   $$PWD/qstatemachine_p.h \
	   $$PWD/qsignaleventgenerator_p.h \
	   $$PWD/qabstractstate.h \
	   $$PWD/qabstractstate_p.h \
	   $$PWD/qstate.h \
	   $$PWD/qstate_p.h \
	   $$PWD/qfinalstate.h \
	   $$PWD/qhistorystate.h \
	   $$PWD/qhistorystate_p.h \
	   $$PWD/qabstracttransition.h \
	   $$PWD/qabstracttransition_p.h \
	   $$PWD/qsignaltransition.h \
	   $$PWD/qsignaltransition_p.h

SOURCES += $$PWD/qstatemachine.cpp \
	   $$PWD/qabstractstate.cpp \
	   $$PWD/qstate.cpp \
	   $$PWD/qfinalstate.cpp \
	   $$PWD/qhistorystate.cpp \
	   $$PWD/qabstracttransition.cpp \
	   $$PWD/qsignaltransition.cpp

!contains(DEFINES, QT_NO_STATEMACHINE_EVENTFILTER) {
HEADERS += $$PWD/qeventtransition.h \
	   $$PWD/qeventtransition_p.h
SOURCES += $$PWD/qeventtransition.cpp
}
