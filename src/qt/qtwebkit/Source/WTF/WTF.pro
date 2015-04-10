# -------------------------------------------------------------------
# Project file for WTF
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------
TEMPLATE = lib
TARGET = WTF

include(WTF.pri)

CONFIG += staticlib optimize_full

VPATH += $$PWD/wtf

INCLUDEPATH += $$PWD/wtf

wince* {
    # for mt19937ar.c
    INCLUDEPATH += $${ROOT_WEBKIT_DIR}/Source/ThirdParty
}

HEADERS += \
    Alignment.h \
    ArrayBuffer.h \
    ArrayBufferView.h \
    ASCIICType.h \
    Assertions.h \
    Atomics.h \
    AVLTree.h \
    Bitmap.h \
    BitArray.h \
    BitVector.h \
    BloomFilter.h \
    BoundsCheckedPointer.h \
    BumpPointerAllocator.h \
    ByteOrder.h \
    CheckedArithmetic.h \
    Compiler.h \
    CryptographicallyRandomNumber.h \
    CurrentTime.h \
    DateMath.h \
    DecimalNumber.h \
    Decoder.h \
    DataLog.h \ 
    Deque.h \
    DisallowCType.h \
    dtoa.h \
    dtoa/bignum-dtoa.h \
    dtoa/bignum.h \
    dtoa/cached-powers.h \
    dtoa/diy-fp.h \
    dtoa/double-conversion.h \
    dtoa/double.h \
    dtoa/fast-dtoa.h \
    dtoa/fixed-dtoa.h \
    dtoa/strtod.h \
    dtoa/utils.h \
    DynamicAnnotations.h \
    Encoder.h \
    EnumClass.h \
    ExportMacros.h \
    FastAllocBase.h \
    FastMalloc.h \
    FeatureDefines.h \
    FilePrintStream.h \
    FixedArray.h \
    Float32Array.h \
    Float64Array.h \
    Forward.h \
    FunctionDispatcher.h \
    Functional.h \
    GetPtr.h \
    GregorianDateTime.h \
    HashCountedSet.h \
    HashFunctions.h \
    HashIterators.h \
    HashMap.h \
    HashSet.h \
    HashTable.h \
    HashTraits.h \
    HexNumber.h \
    Int16Array.h \
    Int32Array.h \
    Int8Array.h \
    ListHashSet.h \
    Locker.h \
    MainThread.h \
    MathExtras.h \
    MD5.h \
    MediaTime.h \
    MessageQueue.h \
    MetaAllocator.h \
    MetaAllocatorHandle.h \
    Noncopyable.h \
    NonCopyingSort.h \
    NotFound.h \
    NullPtr.h \
    NumberOfCores.h \
    RAMSize.h \
    OSAllocator.h \
    OSRandomSource.h \
    OwnArrayPtr.h \
    OwnPtr.h \
    OwnPtrCommon.h \
    PackedIntVector.h \
    PageAllocation.h \
    PageAllocationAligned.h \
    PageBlock.h \
    PageReservation.h \
    ParallelJobs.h \
    ParallelJobsGeneric.h \
    ParallelJobsLibdispatch.h \
    ParallelJobsOpenMP.h \
    PassOwnArrayPtr.h \
    PassOwnPtr.h \
    PassRefPtr.h \
    PassTraits.h \
    Platform.h \
    PossiblyNull.h \
    PrintStream.h \
    ProcessID.h \
    RandomNumber.h \
    RandomNumberSeed.h \
    RawPointer.h \
    RedBlackTree.h \
    RefCounted.h \
    RefCountedLeakCounter.h \
    RefPtr.h \
    RefPtrHashMap.h \
    RetainPtr.h \
    SHA1.h \
    SaturatedArithmetic.h \
    Spectrum.h \
    StackBounds.h \
    StaticConstructors.h \
    StdLibExtras.h \
    StringExtras.h \
    StringHasher.h \
    StringPrintStream.h \
    TCPackedCache.h \
    TCSpinLock.h \
    TCSystemAlloc.h \
    text/ASCIIFastPath.h \
    text/AtomicString.h \
    text/AtomicStringHash.h \
    text/AtomicStringImpl.h \
    text/AtomicStringTable.h \
    text/Base64.h \
    text/CString.h \
    text/IntegerToStringConversion.h \
    text/StringBuffer.h \
    text/StringBuilder.h \
    text/StringConcatenate.h \
    text/StringHash.h \
    text/StringImpl.h \
    text/StringOperators.h \
    text/TextPosition.h \
    text/WTFString.h \
    threads/BinarySemaphore.h \
    Threading.h \
    ThreadingPrimitives.h \
    ThreadRestrictionVerifier.h \
    ThreadSafeRefCounted.h \
    ThreadSpecific.h \
    TypeTraits.h \
    Uint16Array.h \
    Uint32Array.h \
    Uint8Array.h \
    Uint8ClampedArray.h \
    unicode/CharacterNames.h \
    unicode/Collator.h \
    unicode/icu/UnicodeIcu.h \
    unicode/ScriptCodesFromICU.h \
    unicode/Unicode.h \
    unicode/UnicodeMacrosFromICU.h \
    unicode/UTF8.h \
    ValueCheck.h \
    Vector.h \
    VectorTraits.h \
    VMTags.h \
    WTFThreadData.h \
    WeakPtr.h

unix: HEADERS += ThreadIdentifierDataPthreads.h

SOURCES += \
    ArrayBuffer.cpp \
    ArrayBufferView.cpp \
    Assertions.cpp \
    BitVector.cpp \
    CryptographicallyRandomNumber.cpp \
    CurrentTime.cpp \
    DateMath.cpp \
    DataLog.cpp \
    DecimalNumber.cpp \
    dtoa.cpp \
    dtoa/bignum-dtoa.cc \
    dtoa/bignum.cc \
    dtoa/cached-powers.cc \
    dtoa/diy-fp.cc \
    dtoa/double-conversion.cc \
    dtoa/fast-dtoa.cc \
    dtoa/fixed-dtoa.cc \
    dtoa/strtod.cc \
    FastMalloc.cpp \
    FilePrintStream.cpp \
    FunctionDispatcher.cpp \
    GregorianDateTime.cpp \
    gobject/GOwnPtr.cpp \
    gobject/GRefPtr.cpp \
    HashTable.cpp \
    MD5.cpp \
    MainThread.cpp \
    MediaTime.cpp \
    MetaAllocator.cpp \
    NullPtr.cpp \
    NumberOfCores.cpp \
    RAMSize.cpp \
    OSRandomSource.cpp \
    qt/MainThreadQt.cpp \
    qt/StringQt.cpp \
    PageAllocationAligned.cpp \
    PageBlock.cpp \
    ParallelJobsGeneric.cpp \
    PrintStream.cpp \
    RandomNumber.cpp \
    RefCountedLeakCounter.cpp \
    SHA1.cpp \
    StackBounds.cpp \
    StringPrintStream.cpp \
    TCSystemAlloc.cpp \
    Threading.cpp \
    TypeTraits.cpp \
    WTFThreadData.cpp \
    text/AtomicString.cpp \
    text/AtomicStringTable.cpp \
    text/Base64.cpp \
    text/CString.cpp \
    text/StringBuilder.cpp \
    text/StringImpl.cpp \
    text/StringStatics.cpp \
    text/WTFString.cpp \
    unicode/CollatorDefault.cpp \
    unicode/icu/CollatorICU.cpp \
    unicode/UTF8.cpp

unix: SOURCES += \
    OSAllocatorPosix.cpp \
    ThreadIdentifierDataPthreads.cpp \
    ThreadingPthreads.cpp

win*|wince*: SOURCES += \
    win/OwnPtrWin.cpp \
    OSAllocatorWin.cpp \
    ThreadSpecificWin.cpp \
    ThreadingWin.cpp

win32 {
    HEADERS += config.h
    SOURCES += \
        threads/win/BinarySemaphoreWin.cpp
    INCLUDEPATH += $$PWD/wtf/threads
} else {
    SOURCES += \
        threads/BinarySemaphore.cpp
}

QT += core
QT -= gui

*sh4* {
    QMAKE_CXXFLAGS += -mieee -w
    QMAKE_CFLAGS   += -mieee -w
}

*-g++*:lessThan(QT_GCC_MAJOR_VERSION, 5):lessThan(QT_GCC_MINOR_VERSION, 6) {
    # For GCC 4.5 and before we disable C++0x mode in JSC for if enabled in Qt's mkspec
    QMAKE_CXXFLAGS -= -std=c++0x -std=gnu++0x -std=c++11 -std=gnu++11
}

