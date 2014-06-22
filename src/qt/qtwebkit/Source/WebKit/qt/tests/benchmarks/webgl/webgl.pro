include(../../tests.pri)
exists($${TARGET}.qrc):RESOURCES += $${TARGET}.qrc
QT += opengl
