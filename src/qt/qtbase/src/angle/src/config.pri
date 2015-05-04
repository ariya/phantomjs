# This file contains build options that are relevant for both the compilers
# and the khronos implementation libraries.

ANGLE_DIR = $$(ANGLE_DIR)
isEmpty(ANGLE_DIR) {
    ANGLE_DIR = $$absolute_path(../../3rdparty/angle)
} else {
    !build_pass:message("Using external ANGLE from $$ANGLE_DIR")
}

!exists($$ANGLE_DIR/src) {
    error("$$ANGLE_DIR does not contain ANGLE")
}

equals(QMAKE_HOST.os, Windows) {
    gnutools.value = $$absolute_path(../../../../gnuwin32/bin)
    exists($$gnutools.value/gperf.exe) {
        gnutools.name = PATH
        gnutools.CONFIG = prepend
    }
}

defineReplace(addGnuPath) {
    gnuPath = $$1
    !isEmpty(gnuPath):!isEmpty(gnutools.name) {
        qtAddToolEnv(gnuPath, gnutools)
        silent: gnuPath = @echo generating sources from ${QMAKE_FILE_IN} && $$gnuPath
    }
    return($$gnuPath)
}

# Defines for modifying Win32 headers
DEFINES +=  _WINDOWS \
            _UNICODE \
            _CRT_SECURE_NO_DEPRECATE \
            _HAS_EXCEPTIONS=0 \
            NOMINMAX \
            WIN32_LEAN_AND_MEAN=1

!winrt: DEFINES += ANGLE_ENABLE_D3D9 ANGLE_SKIP_DXGI_1_2_CHECK

CONFIG += angle_d3d11 # Remove to disable D3D11 renderer

equals(QMAKE_TARGET_OS, xp): CONFIG -= angle_d3d11

angle_d3d11 {
    DEFINES += ANGLE_ENABLE_D3D11 ANGLE_DEFAULT_D3D11=1
    !build_pass: message("Enabling D3D11 mode for ANGLE")
}

CONFIG(debug, debug|release) {
    DEFINES += _DEBUG
} else {
    !static: CONFIG += rtti_off
    DEFINES += NDEBUG
}

# c++11 is needed by MinGW to get support for unordered_map.
CONFIG += stl exceptions c++11

INCLUDEPATH += . .. $$PWD/../include

msvc {
    # Disabled Warnings:
    #   4100: 'identifier' : unreferenced formal parameter
    #   4127: conditional expression is constant
    #   4189: 'identifier' : local variable is initialized but not referenced
    #   4239: nonstandard extension used : 'token' : conversion from 'type' to 'type'
    #   4244: 'argument' : conversion from 'type1' to 'type2', possible loss of data
    #   4245: 'conversion' : conversion from 'type1' to 'type2', signed/unsigned mismatch
    #   4267: coversion from 'size_t' to 'int', possible loss of data
    #   4275: non - DLL-interface classkey 'identifier' used as base for DLL-interface classkey 'identifier'
    #   4512: 'class' : assignment operator could not be generated
    #   4702: unreachable code
    QMAKE_CFLAGS_WARN_ON    -= -W3
    QMAKE_CFLAGS_WARN_ON    += -W4 -wd"4100" -wd"4127" -wd"4189" -wd"4239" -wd"4244" -wd"4245" -wd"4267" -wd"4275" -wd"4512" -wd"4702"
    # Optimizations
    #   /Oy:   Omits frame pointer (x86 only).
    #   /Gy:   Enables function-level linking.
    #   /GS:   Buffers security check.
    #   /Gm-:  Disable minimal rebuild.
    #   /RTC1: Run time error checking
    QMAKE_CFLAGS_RELEASE    += -Oy- -Gy -GS -Gm-
    QMAKE_CFLAGS_DEBUG      += -Oy- -Gy -GS -Gm- -RTC1
    QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO = -Zi $$QMAKE_CFLAGS_RELEASE

    QMAKE_CXXFLAGS_WARN_ON = $$QMAKE_CFLAGS_WARN_ON
}

gcc {
    QMAKE_CFLAGS_WARN_ON += -Wno-unknown-pragmas -Wno-comment -Wno-missing-field-initializers \
                            -Wno-switch -Wno-unused-parameter -Wno-write-strings -Wno-sign-compare -Wno-missing-braces \
                            -Wno-unused-but-set-variable -Wno-unused-variable -Wno-narrowing -Wno-maybe-uninitialized \
                            -Wno-strict-aliasing -Wno-type-limits -Wno-unused-local-typedefs

    QMAKE_CXXFLAGS_WARN_ON = $$QMAKE_CFLAGS_WARN_ON -Wno-reorder -Wno-conversion-null -Wno-delete-non-virtual-dtor

    sse2: QMAKE_CXXFLAGS += -march=native
}

QMAKE_CXXFLAGS_DEBUG = $$QMAKE_CFLAGS_DEBUG
QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE

load(qt_helper_lib)
