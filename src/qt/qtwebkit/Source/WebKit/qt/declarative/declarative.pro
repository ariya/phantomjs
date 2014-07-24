# -------------------------------------------------------------------
# Main project file for the Qt Quick (QML) plugin
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered

public_api.file = public.pri
public_api.makefile = Makefile.declarative.public
SUBDIRS += public_api

build?(webkit2): {
    experimental_api.file = experimental/experimental.pri
    experimental_api.makefile = Makefile.declarative.experimental
    SUBDIRS += experimental_api
}
