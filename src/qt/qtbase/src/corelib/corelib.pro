TARGET	   = QtCore
QT         =
CONFIG    += exceptions

MODULE = core     # not corelib, as per project file
MODULE_CONFIG = moc resources
!isEmpty(QT_NAMESPACE): MODULE_DEFINES = QT_NAMESPACE=$$QT_NAMESPACE

CONFIG += $$MODULE_CONFIG
DEFINES   += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x67000000
irix-cc*:QMAKE_CXXFLAGS += -no_prelink -ptused

CONFIG += optimize_full

# otherwise mingw headers do not declare common functions like putenv
mingw:QMAKE_CXXFLAGS_CXX11 = -std=gnu++0x

QMAKE_DOCS = $$PWD/doc/qtcore.qdocconf

ANDROID_JAR_DEPENDENCIES = \
    jar/QtAndroid.jar \
    jar/QtAndroidAccessibility.jar
ANDROID_LIB_DEPENDENCIES = \
    plugins/platforms/android/libqtforandroid.so
ANDROID_BUNDLED_JAR_DEPENDENCIES = \
    jar/QtAndroid-bundled.jar \
    jar/QtAndroidAccessibility-bundled.jar
ANDROID_PERMISSIONS = \
    android.permission.INTERNET \
    android.permission.WRITE_EXTERNAL_STORAGE

load(qt_module)

include(animation/animation.pri)
include(arch/arch.pri)
include(global/global.pri)
include(thread/thread.pri)
include(tools/tools.pri)
include(io/io.pri)
include(itemmodels/itemmodels.pri)
include(json/json.pri)
include(plugin/plugin.pri)
include(kernel/kernel.pri)
include(codecs/codecs.pri)
include(statemachine/statemachine.pri)
include(mimetypes/mimetypes.pri)
include(xml/xml.pri)

mac|darwin {
    !ios {
        LIBS_PRIVATE += -framework ApplicationServices
        LIBS_PRIVATE += -framework CoreServices
    }
    LIBS_PRIVATE += -framework CoreFoundation
    LIBS_PRIVATE += -framework Foundation
}
win32:DEFINES-=QT_NO_CAST_TO_ASCII
DEFINES += $$MODULE_DEFINES

QMAKE_LIBS += $$QMAKE_LIBS_CORE

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtCore.dynlist

contains(DEFINES,QT_EVAL):include(eval.pri)

HOST_BINS = $$[QT_HOST_BINS]
host_bins.name = host_bins
host_bins.variable = HOST_BINS

qt_conf.name = qt_config
qt_conf.variable = QT_CONFIG

QMAKE_PKGCONFIG_VARIABLES += host_bins qt_conf

ctest_macros_file.input = $$PWD/Qt5CTestMacros.cmake
ctest_macros_file.output = $$DESTDIR/cmake/Qt5Core/Qt5CTestMacros.cmake
ctest_macros_file.CONFIG = verbatim

cmake_umbrella_config_file.input = $$PWD/Qt5Config.cmake.in
cmake_umbrella_config_file.output = $$DESTDIR/cmake/Qt5/Qt5Config.cmake

cmake_umbrella_config_version_file.input = $$PWD/../../mkspecs/features/data/cmake/Qt5ConfigVersion.cmake.in
cmake_umbrella_config_version_file.output = $$DESTDIR/cmake/Qt5/Qt5ConfigVersion.cmake

load(cmake_functions)
load(qfeatures)

CMAKE_DISABLED_FEATURES = $$join(QT_DISABLED_FEATURES, "$$escape_expand(\\n)    ")

CMAKE_HOST_DATA_DIR = $$cmakeRelativePath($$[QT_HOST_DATA/src], $$[QT_INSTALL_PREFIX])
contains(CMAKE_HOST_DATA_DIR, "^\\.\\./.*"):!isEmpty(CMAKE_HOST_DATA_DIR) {
    CMAKE_HOST_DATA_DIR = $$[QT_HOST_DATA/src]/
    CMAKE_HOST_DATA_DIR_IS_ABSOLUTE = True
}

cmake_extras_mkspec_dir.input = $$PWD/Qt5CoreConfigExtrasMkspecDir.cmake.in
cmake_extras_mkspec_dir.output = $$DESTDIR/cmake/Qt5Core/Qt5CoreConfigExtrasMkspecDir.cmake

CMAKE_INSTALL_DATA_DIR = $$cmakeRelativePath($$[QT_HOST_DATA], $$[QT_INSTALL_PREFIX])
contains(CMAKE_INSTALL_DATA_DIR, "^\\.\\./.*"):!isEmpty(CMAKE_INSTALL_DATA_DIR) {
    CMAKE_INSTALL_DATA_DIR = $$[QT_HOST_DATA]/
    CMAKE_INSTALL_DATA_DIR_IS_ABSOLUTE = True
}

cmake_extras_mkspec_dir_for_install.input = $$PWD/Qt5CoreConfigExtrasMkspecDirForInstall.cmake.in
cmake_extras_mkspec_dir_for_install.output = $$DESTDIR/cmake/install/Qt5Core/Qt5CoreConfigExtrasMkspecDir.cmake

cmake_qt5_umbrella_module_files.files = $$cmake_umbrella_config_file.output $$cmake_umbrella_config_version_file.output
cmake_qt5_umbrella_module_files.path = $$[QT_INSTALL_LIBS]/cmake/Qt5

QMAKE_SUBSTITUTES += ctest_macros_file cmake_umbrella_config_file cmake_umbrella_config_version_file cmake_extras_mkspec_dir cmake_extras_mkspec_dir_for_install

ctest_qt5_module_files.files += $$ctest_macros_file.output $$cmake_extras_mkspec_dir_for_install.output

ctest_qt5_module_files.path = $$[QT_INSTALL_LIBS]/cmake/Qt5Core

INSTALLS += ctest_qt5_module_files cmake_qt5_umbrella_module_files

mips_dsp:*-g++* {
    HEADERS += $$MIPS_DSP_HEADERS

    mips_dsp_corelib_assembler.commands = $$QMAKE_CXX -c
    mips_dsp_corelib_assembler.commands += $(CXXFLAGS) $(INCPATH) -mips32r2 -mdsp ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
    mips_dsp_corelib_assembler.dependency_type = TYPE_C
    mips_dsp_corelib_assembler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
    mips_dsp_corelib_assembler.input = MIPS_DSP_ASM
    mips_dsp_corelib_assembler.variable_out = OBJECTS
    mips_dsp_corelib_assembler.name = assembling[mips_dsp] ${QMAKE_FILE_IN}
    silent:mips_dsp_corelib_assembler.commands = @echo assembling[mips_dsp] ${QMAKE_FILE_IN} && $$mips_dsp_corelib_assembler.commands
    QMAKE_EXTRA_COMPILERS += mips_dsp_corelib_assembler
}
