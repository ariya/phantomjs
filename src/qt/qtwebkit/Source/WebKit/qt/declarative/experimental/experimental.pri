# -------------------------------------------------------------------
# Project file for the Qt Quick (QML) experimental API plugin
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = lib
TARGET  = qmlwebkitexperimentalplugin

TARGET.module_name = QtWebKit/experimental

CONFIG += plugin

QMLDIRFILE = $${_PRO_FILE_PWD_}/qmldir
copy2build.input = QMLDIRFILE
copy2build.output = $${ROOT_BUILD_DIR}/imports/$${TARGET.module_name}/qmldir
!contains(TEMPLATE_PREFIX, vc):copy2build.variable_out = PRE_TARGETDEPS
copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
copy2build.name = COPY ${QMAKE_FILE_IN}
copy2build.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += copy2build

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

wince*:LIBS += $$QMAKE_LIBS_GUI

QT += network quick quick-private webkit webkit-private

DESTDIR = $${ROOT_BUILD_DIR}/imports/$${TARGET.module_name}

CONFIG += rpath
RPATHDIR_RELATIVE_TO_DESTDIR = ../../lib

SOURCES += plugin.cpp

DEFINES += HAVE_WEBKIT2

WEBKIT += wtf javascriptcore webkit2

# The fallback to QT_INSTALL_IMPORTS can be removed once we
# depend on Qt 5 RC1.
importPath = $$[QT_INSTALL_QML]
isEmpty(importPath): importPath = $$[QT_INSTALL_IMPORTS]

target.path = $${importPath}/$${TARGET.module_name}


qmldir.files += $$PWD/qmldir
qmldir.path +=  $${importPath}/$${TARGET.module_name}

INSTALLS += target qmldir


