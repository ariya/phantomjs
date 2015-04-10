include(../tests.pri)
exists($${TARGET}.qrc):RESOURCES += $${TARGET}.qrc

enable?(WEBGL) {
    QT += opengl
}
