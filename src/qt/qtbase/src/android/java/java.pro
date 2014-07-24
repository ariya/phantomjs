CONFIG -= qt android_install

javaresources.files = \
    $$PWD/AndroidManifest.xml \
    $$PWD/version.xml \
    $$PWD/res \
    $$PWD/src

javaresources.path = $$[QT_INSTALL_PREFIX]/src/android/java

INSTALLS += javaresources

!prefix_build:!equals(OUT_PWD, $$PWD) {
    RETURN = $$escape_expand(\\n\\t)
    equals(QMAKE_HOST.os, Windows) {
        RETURN = $$escape_expand(\\r\\n\\t)
    }
    OUT_PATH = $$shell_path($$OUT_PWD)

    QMAKE_POST_LINK += \
        $${QMAKE_COPY} $$shell_path($$PWD/AndroidManifest.xml) $$OUT_PATH $$RETURN \
        $${QMAKE_COPY} $$shell_path($$PWD/version.xml) $$OUT_PATH $$RETURN \
        $${QMAKE_COPY_DIR} $$shell_path($$PWD/res) $$OUT_PATH $$RETURN \
        $${QMAKE_COPY_DIR} $$shell_path($$PWD/src) $$OUT_PATH
}
