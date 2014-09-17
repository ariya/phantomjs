TEMPLATE = lib
isEmpty(QT_MAJOR_VERSION) {
   VERSION=4.8.4
} else {
   VERSION=$${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION}
}
CONFIG += qt plugin

win32|mac:!wince*:!win32-msvc:!macx-xcode:CONFIG += debug_and_release
TARGET = $$qtLibraryTarget($$TARGET)
contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

include(../qt_targets.pri)

wince*:LIBS += $$QMAKE_LIBS_GUI

symbian: {
    TARGET.EPOCALLOWDLLDATA=1
    TARGET.CAPABILITY = All -Tcb
    TARGET = $${TARGET}$${QT_LIBINFIX}
    load(armcc_warnings)

    # Make partial upgrade SIS file for Qt plugin dll's
    # Partial upgrade SIS file
    vendorinfo = \
        "; Localised Vendor name" \
        "%{\"Nokia\"}" \
        " " \
        "; Unique Vendor name" \
        ":\"Nokia, Qt\"" \
        " "
    isEmpty(QT_LIBINFIX): PARTIAL_UPGRADE_UID = 0x2001E61C
    else: PARTIAL_UPGRADE_UID = 0xE001E61C

    pu_header = "; Partial upgrade package for testing $${TARGET} changes without reinstalling everything" \
                "$${LITERAL_HASH}{\"$${TARGET}\"}, ($$PARTIAL_UPGRADE_UID), $${QT_MAJOR_VERSION},$${QT_MINOR_VERSION},$${QT_PATCH_VERSION}, TYPE=PU,RU"
    partial_upgrade.pkg_prerules = pu_header vendorinfo
    partial_upgrade.files = $$QMAKE_LIBDIR_QT/$${TARGET}.dll
    partial_upgrade.path = c:/sys/bin
    DEPLOYMENT += partial_upgrade
}
