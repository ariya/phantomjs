
DBUS_ADAPTORS = $$PWD/xml/Cache.xml $$PWD/xml/DeviceEventController.xml
QDBUSXML2CPP_ADAPTOR_HEADER_FLAGS = -i struct_marshallers_p.h

DBUS_INTERFACES = $$PWD/xml/Socket.xml $$PWD/xml/Bus.xml
QDBUSXML2CPP_INTERFACE_HEADER_FLAGS = -i struct_marshallers_p.h

INCLUDEPATH += $$PWD
HEADERS += $$PWD/atspi/atspi-constants.h

