option(host_build)

MODULE = bootstrap_dbus
TARGET = QtBootstrapDBus
CONFIG += no_module_headers internal_module force_bootstrap

DEFINES += \
    QT_NO_CAST_FROM_ASCII

MODULE_INCLUDES = \
    \$\$QT_MODULE_INCLUDE_BASE/QtDBus
MODULE_PRIVATE_INCLUDES = \
    \$\$QT_MODULE_INCLUDE_BASE/QtDBus/$$QT_VERSION \
    \$\$QT_MODULE_INCLUDE_BASE/QtDBus/$$QT_VERSION/QtDBus

load(qt_module)

QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS

SOURCES = \
    ../../dbus/qdbusintrospection.cpp \
    ../../dbus/qdbusxmlparser.cpp \
    ../../dbus/qdbuserror.cpp \
    ../../dbus/qdbusutil.cpp \
    ../../dbus/qdbusmisc.cpp \
    ../../dbus/qdbusmetatype.cpp \
    ../../dbus/qdbusargument.cpp \
    ../../dbus/qdbusextratypes.cpp \
    ../../dbus/qdbus_symbols.cpp \
    ../../dbus/qdbusunixfiledescriptor.cpp

lib.CONFIG = dummy_install
INSTALLS = lib
