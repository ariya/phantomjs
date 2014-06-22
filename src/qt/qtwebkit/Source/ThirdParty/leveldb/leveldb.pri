# -------------------------------------------------------------------
# This file contains shared rules used both when building leveldb
# itself, and by targets that use leveldb.
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

SOURCE_DIR = $${ROOT_WEBKIT_DIR}/Source/ThirdParty/leveldb

INCLUDEPATH += \
    $$SOURCE_DIR/include \
    $$SOURCE_DIR

