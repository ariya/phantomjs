
ARCH_SUBDIR=x86
contains(QMAKE_TARGET.arch, x86_64): {
    ARCH_SUBDIR=amd64
} else {
    !contains(QMAKE_TARGET.arch, x86): message("ERROR: Could not detect architecture from QMAKE_TARGET.arch")
}

MIDL_GENERATED = $$PWD/generated/$${ARCH_SUBDIR}

INCLUDEPATH += $$MIDL_GENERATED

SOURCES +=  $${MIDL_GENERATED}/Accessible2_p.c \
            $${MIDL_GENERATED}/AccessibleAction_p.c \
            $${MIDL_GENERATED}/AccessibleApplication_p.c \
            $${MIDL_GENERATED}/AccessibleComponent_p.c \
            $${MIDL_GENERATED}/AccessibleEditableText_p.c \
            $${MIDL_GENERATED}/AccessibleHyperlink_p.c \
            $${MIDL_GENERATED}/AccessibleHypertext_p.c \
            $${MIDL_GENERATED}/AccessibleImage_p.c \
            $${MIDL_GENERATED}/AccessibleRelation_p.c \
            $${MIDL_GENERATED}/AccessibleTable2_p.c \
            $${MIDL_GENERATED}/AccessibleTableCell_p.c \
            $${MIDL_GENERATED}/AccessibleTable_p.c \
            $${MIDL_GENERATED}/AccessibleText_p.c \
            $${MIDL_GENERATED}/AccessibleValue_p.c

SOURCES +=  $${MIDL_GENERATED}/Accessible2_i.c \
            $${MIDL_GENERATED}/AccessibleAction_i.c \
            $${MIDL_GENERATED}/AccessibleApplication_i.c \
            $${MIDL_GENERATED}/AccessibleComponent_i.c \
            $${MIDL_GENERATED}/AccessibleEditableText_i.c \
            $${MIDL_GENERATED}/AccessibleHyperlink_i.c \
            $${MIDL_GENERATED}/AccessibleHypertext_i.c \
            $${MIDL_GENERATED}/AccessibleImage_i.c \
            $${MIDL_GENERATED}/AccessibleRelation_i.c \
            $${MIDL_GENERATED}/AccessibleTable2_i.c \
            $${MIDL_GENERATED}/AccessibleTableCell_i.c \
            $${MIDL_GENERATED}/AccessibleTable_i.c \
            $${MIDL_GENERATED}/AccessibleText_i.c \
            $${MIDL_GENERATED}/AccessibleValue_i.c

SOURCES +=  $${MIDL_GENERATED}/IA2TypeLibrary_i.c

# Do not add dlldata.c when building accessibility into a static library, as the COM entry points
# defined there can cause duplicate symbol errors when linking into a binary that also defines
# such entry points, e.g. anything linked against QtAxServer.
!static: SOURCES += $${MIDL_GENERATED}/dlldata.c

HEADERS +=  $${MIDL_GENERATED}/Accessible2.h \
            $${MIDL_GENERATED}/AccessibleAction.h \
            $${MIDL_GENERATED}/AccessibleApplication.h \
            $${MIDL_GENERATED}/AccessibleComponent.h \
            $${MIDL_GENERATED}/AccessibleEditableText.h \
            $${MIDL_GENERATED}/AccessibleEventID.h \
            $${MIDL_GENERATED}/AccessibleHyperlink.h \
            $${MIDL_GENERATED}/AccessibleHypertext.h \
            $${MIDL_GENERATED}/AccessibleImage.h \
            $${MIDL_GENERATED}/AccessibleRelation.h \
            $${MIDL_GENERATED}/AccessibleRole.h \
            $${MIDL_GENERATED}/AccessibleStates.h \
            $${MIDL_GENERATED}/AccessibleTable.h \
            $${MIDL_GENERATED}/AccessibleTable2.h \
            $${MIDL_GENERATED}/AccessibleTableCell.h \
            $${MIDL_GENERATED}/AccessibleText.h \
            $${MIDL_GENERATED}/AccessibleValue.h \
            $${MIDL_GENERATED}/IA2CommonTypes.h \
            $${MIDL_GENERATED}/IA2TypeLibrary.h


OTHER_FILES = \
    $$PWD/idl/Accessible2.idl \
    $$PWD/idl/AccessibleAction.idl \
    $$PWD/idl/AccessibleApplication.idl \
    $$PWD/idl/AccessibleComponent.idl \
    $$PWD/idl/AccessibleEditableText.idl \
    $$PWD/idl/AccessibleEventID.idl \
    $$PWD/idl/AccessibleHyperlink.idl \
    $$PWD/idl/AccessibleHypertext.idl \
    $$PWD/idl/AccessibleImage.idl \
    $$PWD/idl/AccessibleRelation.idl \
    $$PWD/idl/AccessibleRole.idl \
    $$PWD/idl/AccessibleStates.idl \
    $$PWD/idl/AccessibleTable.idl \
    $$PWD/idl/AccessibleTable2.idl \
    $$PWD/idl/AccessibleTableCell.idl \
    $$PWD/idl/AccessibleText.idl \
    $$PWD/idl/AccessibleValue.idl \
    $$PWD/idl/IA2CommonTypes.idl \
    $$PWD/idl/IA2TypeLibrary.idl

LIBS += -lrpcrt4

TR_EXCLUDE += $$PWD/*
