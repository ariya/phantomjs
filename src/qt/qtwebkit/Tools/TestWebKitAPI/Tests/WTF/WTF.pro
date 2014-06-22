TEMPLATE = app
TARGET = tst_wtf

SOURCES += \
    AtomicString.cpp \
    CheckedArithmeticOperations.cpp \
    CString.cpp \
    Functional.cpp \
    HashMap.cpp \
    HashSet.cpp \
    IntegerToStringConversion.cpp \
    ListHashSet.cpp \
    MD5.cpp \
    MathExtras.cpp \
    MediaTime.cpp \
    RedBlackTree.cpp \
    SHA1.cpp \
    SaturatedArithmeticOperations.cpp \
    StringBuilder.cpp \
    StringHasher.cpp \
    StringImpl.cpp \
    StringOperators.cpp \
    TemporaryChange.cpp \
    Vector.cpp \
    VectorBasic.cpp \
    VectorReverse.cpp \
    WTFString.cpp

include(../../TestWebKitAPI.pri)

DEFINES += APITEST_SOURCE_DIR=\\\"$$PWD\\\"

