TEMPLIBS = $$[QT_INSTALL_LIBS] libQtGui.prl
include($$join(TEMPLIBS, "/"))

contains(QMAKE_PRL_CONFIG, shared) {
    message(Shared Qt)
} else {
    message(Static Qt)
}
