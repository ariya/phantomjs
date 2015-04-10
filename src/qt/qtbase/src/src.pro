TEMPLATE = subdirs

src_tools_bootstrap.subdir = tools/bootstrap
src_tools_bootstrap.target = sub-bootstrap
src_tools_bootstrap.CONFIG = host_build

src_tools_moc.subdir = tools/moc
src_tools_moc.target = sub-moc
src_tools_moc.depends = src_tools_bootstrap
src_tools_moc.CONFIG = host_build

src_tools_rcc.subdir = tools/rcc
src_tools_rcc.target = sub-rcc
src_tools_rcc.depends = src_tools_bootstrap
src_tools_rcc.CONFIG = host_build

src_tools_qlalr.subdir = tools/qlalr
src_tools_qlalr.target = sub-qlalr
force_bootstrap: src_tools_qlalr.depends = src_tools_bootstrap
else: src_tools_qlalr.depends = src_corelib

src_tools_uic.subdir = tools/uic
src_tools_uic.target = sub-uic
src_tools_uic.CONFIG = host_build
force_bootstrap: src_tools_uic.depends = src_tools_bootstrap
else: src_tools_uic.depends = src_corelib

src_tools_qdoc.subdir = tools/qdoc
src_tools_qdoc.target = sub-qdoc
src_tools_qdoc.CONFIG = host_build
force_bootstrap: src_tools_qdoc.depends = src_tools_bootstrap
else: src_tools_qdoc.depends = src_corelib src_xml

src_tools_bootstrap_dbus.subdir = tools/bootstrap-dbus
src_tools_bootstrap_dbus.target = sub-bootstrap_dbus
src_tools_bootstrap_dbus.depends = src_tools_bootstrap
src_tools_bootstrap_dbus.CONFIG = host_build

src_tools_qdbusxml2cpp.subdir = tools/qdbusxml2cpp
src_tools_qdbusxml2cpp.target = sub-qdbusxml2cpp
src_tools_qdbusxml2cpp.CONFIG = host_build
force_bootstrap: src_tools_qdbusxml2cpp.depends = src_tools_bootstrap_dbus
else: src_tools_qdbusxml2cpp.depends = src_dbus

src_tools_qdbuscpp2xml.subdir = tools/qdbuscpp2xml
src_tools_qdbuscpp2xml.target = sub-qdbuscpp2xml
src_tools_qdbuscpp2xml.CONFIG = host_build
force_bootstrap: src_tools_qdbuscpp2xml.depends = src_tools_bootstrap_dbus
else: src_tools_qdbuscpp2xml.depends = src_dbus

src_winmain.subdir = $$PWD/winmain
src_winmain.target = sub-winmain
src_winmain.depends = sub-corelib  # just for the module .pri file

src_corelib.subdir = $$PWD/corelib
src_corelib.target = sub-corelib
src_corelib.depends = src_tools_moc src_tools_rcc

src_xml.subdir = $$PWD/xml
src_xml.target = sub-xml
src_xml.depends = src_corelib

src_dbus.subdir = $$PWD/dbus
src_dbus.target = sub-dbus
src_dbus.depends = src_corelib

src_concurrent.subdir = $$PWD/concurrent
src_concurrent.target = sub-concurrent
src_concurrent.depends = src_corelib

src_sql.subdir = $$PWD/sql
src_sql.target = sub-sql
src_sql.depends = src_corelib

src_network.subdir = $$PWD/network
src_network.target = sub-network
src_network.depends = src_corelib

src_3rdparty_harfbuzzng.subdir = $$PWD/3rdparty/harfbuzz-ng
src_3rdparty_harfbuzzng.target = sub-3rdparty-harfbuzzng

src_gui.subdir = $$PWD/gui
src_gui.target = sub-gui
src_gui.depends = src_corelib

src_platformsupport.subdir = $$PWD/platformsupport
src_platformsupport.target = sub-platformsupport
src_platformsupport.depends = src_corelib src_gui src_network

src_widgets.subdir = $$PWD/widgets
src_widgets.target = sub-widgets
src_widgets.depends = src_corelib src_gui src_tools_uic

src_printsupport.subdir = $$PWD/printsupport
src_printsupport.target = sub-printsupport
src_printsupport.depends = src_corelib src_gui src_widgets src_tools_uic

src_plugins.subdir = $$PWD/plugins
src_plugins.target = sub-plugins
src_plugins.depends = src_sql src_xml src_network

src_android.subdir = $$PWD/android

# this order is important
SUBDIRS += src_tools_bootstrap src_tools_moc src_tools_rcc src_corelib src_tools_qlalr
TOOLS = src_tools_moc src_tools_rcc src_tools_qlalr
win32:SUBDIRS += src_winmain
SUBDIRS += src_network src_sql src_xml
contains(QT_CONFIG, dbus) {
    SUBDIRS += src_dbus
    force_bootstrap: SUBDIRS += src_tools_bootstrap_dbus
    SUBDIRS += src_tools_qdbusxml2cpp src_tools_qdbuscpp2xml
    TOOLS += src_tools_qdbusxml2cpp src_tools_qdbuscpp2xml
    contains(QT_CONFIG, accessibility-atspi-bridge): \
        src_platformsupport.depends += src_dbus src_tools_qdbusxml2cpp
    src_plugins.depends += src_dbus src_tools_qdbusxml2cpp src_tools_qdbuscpp2xml
}
contains(QT_CONFIG, concurrent):SUBDIRS += src_concurrent
!contains(QT_CONFIG, no-gui) {
    contains(QT_CONFIG, harfbuzz) {
        SUBDIRS += src_3rdparty_harfbuzzng
        src_gui.depends += src_3rdparty_harfbuzzng
    }
    win32:contains(QT_CONFIG, angle)|contains(QT_CONFIG, dynamicgl) {
        SUBDIRS += src_angle
        src_gui.depends += src_angle
    }
    SUBDIRS += src_gui src_platformsupport
    contains(QT_CONFIG, opengl(es2)?):SUBDIRS += src_openglextensions
    src_plugins.depends += src_gui src_platformsupport
    !contains(QT_CONFIG, no-widgets) {
        SUBDIRS += src_tools_uic src_widgets
        TOOLS += src_tools_uic
        src_plugins.depends += src_widgets
        contains(QT_CONFIG, opengl(es2)?):!contains(QT_CONFIG, dynamicgl) {
            SUBDIRS += src_opengl
            src_plugins.depends += src_opengl
        }
        !wince*:!winrt {
            SUBDIRS += src_printsupport
            src_plugins.depends += src_printsupport
        }
    }
}
SUBDIRS += src_plugins src_tools_qdoc

nacl: SUBDIRS -= src_network src_testlib

android:!android-no-sdk: SUBDIRS += src_android

TR_EXCLUDE = \
    src_tools_bootstrap src_tools_moc src_tools_rcc src_tools_uic src_tools_qlalr \
    src_tools_bootstrap_dbus src_tools_qdbusxml2cpp src_tools_qdbuscpp2xml \
    src_3rdparty_harfbuzzng

sub-tools.depends = $$TOOLS
QMAKE_EXTRA_TARGETS = sub-tools
