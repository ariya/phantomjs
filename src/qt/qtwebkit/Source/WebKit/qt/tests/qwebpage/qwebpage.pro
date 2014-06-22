include(../tests.pri)
exists($${TARGET}.qrc):RESOURCES += $${TARGET}.qrc
QT *= core-private gui-private
