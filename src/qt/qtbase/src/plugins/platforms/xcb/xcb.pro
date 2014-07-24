TEMPLATE = subdirs
CONFIG += ordered

contains(QT_CONFIG, xcb-qt):SUBDIRS+=xcb-static
SUBDIRS += xcb-plugin.pro
