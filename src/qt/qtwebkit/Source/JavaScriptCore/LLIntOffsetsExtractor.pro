# -------------------------------------------------------------------
# Project file for the LLIntOffsetsExtractor binary, used to generate
# derived sources for JavaScriptCore.
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = app
TARGET = LLIntOffsetsExtractor

debug_and_release {
    CONFIG += force_build_all
    CONFIG += build_all
}

# Don't try to link against any Qt libraries, but at least
# pull in include paths as we include qglobal.h.
INCLUDEPATH += $$QT.core.includes
CONFIG += console
CONFIG -= qt

defineTest(addIncludePaths) {
    # Just needed for include paths
    include(JavaScriptCore.pri)
    include(../WTF/WTF.pri)

    export(INCLUDEPATH)
}

addIncludePaths()

LLINT_DEPENDENCY = \
    $$PWD/llint/LowLevelInterpreter.asm \
    $$PWD/llint/LowLevelInterpreter32_64.asm \
    $$PWD/llint/LowLevelInterpreter64.asm \
    $$PWD/offlineasm/arm.rb \
    $$PWD/offlineasm/ast.rb \
    $$PWD/offlineasm/backends.rb \
    $$PWD/offlineasm/generate_offset_extractor.rb \
    $$PWD/offlineasm/instructions.rb \
    $$PWD/offlineasm/offsets.rb \
    $$PWD/offlineasm/opt.rb \
    $$PWD/offlineasm/parser.rb \
    $$PWD/offlineasm/registers.rb \
    $$PWD/offlineasm/self_hash.rb \
    $$PWD/offlineasm/settings.rb \
    $$PWD/offlineasm/sh4.rb \
    $$PWD/offlineasm/transform.rb \
    $$PWD/offlineasm/x86.rb

INPUT_FILES = $$PWD/llint/LowLevelInterpreter.asm
llint.output = LLIntDesiredOffsets.h
llint.script = $$PWD/offlineasm/generate_offset_extractor.rb
llint.input = INPUT_FILES
llint.depends = $$LLINT_DEPENDENCY
llint.commands = ruby $$llint.script ${QMAKE_FILE_NAME} ${QMAKE_FILE_OUT}
llint.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += llint

macx {
    DESTDIR = $$targetSubDir()
    llint.output = $$targetSubDir()/$$llint.output
    INCLUDEPATH += $$targetSubDir()
}

# Compilation of this file will automatically depend on LLIntDesiredOffsets.h
# due to qmake scanning the source file for header dependencies.
SOURCES = llint/LLIntOffsetsExtractor.cpp

mac: LIBS_PRIVATE += -framework AppKit
