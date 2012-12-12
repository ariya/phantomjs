TARGET	   = QtCore
QPRO_PWD   = $$PWD
QT         =
DEFINES   += QT_BUILD_CORE_LIB QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x67000000
irix-cc*:QMAKE_CXXFLAGS += -no_prelink -ptused

include(../qbase.pri)
include(animation/animation.pri)
include(arch/arch.pri)
include(concurrent/concurrent.pri)
include(global/global.pri)
include(thread/thread.pri)
include(tools/tools.pri)
include(io/io.pri)
include(plugin/plugin.pri)
include(kernel/kernel.pri)
include(codecs/codecs.pri)
include(statemachine/statemachine.pri)
include(xml/xml.pri)

!qpa:mac|darwin:LIBS_PRIVATE += -framework ApplicationServices
qpa:mac {
    !ios {
        LIBS_PRIVATE += -framework CoreServices -framework Security
    }
} else:mac|darwin {
        LIBS_PRIVATE += -framework CoreFoundation -framework Security
}
mac:lib_bundle:DEFINES += QT_NO_DEBUG_PLUGIN_CHECK
win32:DEFINES-=QT_NO_CAST_TO_ASCII

QMAKE_LIBS += $$QMAKE_LIBS_CORE

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtCore.dynlist

contains(DEFINES,QT_EVAL):include(eval.pri)

symbian: {
    TARGET.UID3=0x2001B2DC

    # Problems using data exports from this DLL mean that we can't page it on releases that don't support
    # data exports (currently that's any release before Symbian^3)
    pagingBlock = "$${LITERAL_HASH}ifndef SYMBIAN_DLL_DATA_EXPORTS_SUPPORTED" \
                  "UNPAGED" \
                  "$${LITERAL_HASH}endif"
    MMP_RULES += pagingBlock
    LIBS += -ltzclient
}
