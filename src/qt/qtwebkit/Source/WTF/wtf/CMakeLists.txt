set(WTF_HEADERS
    ASCIICType.h
    AVLTree.h
    Alignment.h
    Assertions.h
    Atomics.h
    BitArray.h
    BitVector.h
    Bitmap.h
    BoundsCheckedPointer.h
    BumpPointerAllocator.h
    ByteOrder.h
    Compiler.h
    CryptographicallyRandomNumber.h
    CurrentTime.h
    DateMath.h
    DataLog.h
    DecimalNumber.h
    Decoder.h
    Deque.h
    DisallowCType.h
    DoublyLinkedList.h
    DynamicAnnotations.h
    Encoder.h
    FastAllocBase.h
    FastMalloc.h
    FeatureDefines.h
    FilePrintStream.h
    FixedArray.h
    Forward.h
    FunctionDispatcher.h
    Functional.h
    GetPtr.h
    GregorianDateTime.h
    HashCountedSet.h
    HashFunctions.h
    HashIterators.h
    HashMap.h
    HashSet.h
    HashTable.h
    HashTraits.h
    HexNumber.h
    ListHashSet.h
    Locker.h
    MD5.h
    MainThread.h
    MathExtras.h
    MediaTime.h
    MessageQueue.h
    MetaAllocator.h
    MetaAllocatorHandle.h
    NonCopyingSort.h
    ThreadRestrictionVerifier.h
    Noncopyable.h
    NotFound.h
    NullPtr.h
    NumberOfCores.h
    RAMSize.h
    OSAllocator.h
    OSRandomSource.h
    OwnArrayPtr.h
    OwnPtr.h
    OwnPtrCommon.h
    PageAllocation.h
    PageAllocationAligned.h
    PageBlock.h
    PageReservation.h
    PassOwnArrayPtr.h
    PassOwnPtr.h
    PassRefPtr.h
    PassTraits.h
    ParallelJobs.h
    ParallelJobsGeneric.h
    ParallelJobsLibdispatch.h
    ParallelJobsOpenMP.h
    Platform.h
    PossiblyNull.h
    PrintStream.h
    ProcessID.h
    RandomNumber.h
    RandomNumberSeed.h
    RawPointer.h
    RedBlackTree.h
    RefCounted.h
    RefCountedLeakCounter.h
    RefPtr.h
    RefPtrHashMap.h
    RetainPtr.h
    SaturatedArithmetic.h
    SegmentedVector.h
    SHA1.h
    StackBounds.h
    StaticConstructors.h
    StdLibExtras.h
    StringExtras.h
    StringHasher.h
    StringPrintStream.h
    TCPackedCache.h
    TCPageMap.h
    TCSpinLock.h
    TCSystemAlloc.h
    ThreadIdentifierDataPthreads.h
    ThreadSafeRefCounted.h
    ThreadSpecific.h
    Threading.h
    ThreadingPrimitives.h
    TypeTraits.h
    VMTags.h
    ValueCheck.h
    Vector.h
    VectorTraits.h
    WTFThreadData.h
    WeakPtr.h
    dtoa.h

    dtoa/bignum-dtoa.h
    dtoa/bignum.h
    dtoa/cached-powers.h
    dtoa/diy-fp.h
    dtoa/double-conversion.h
    dtoa/double.h
    dtoa/fast-dtoa.h
    dtoa/fixed-dtoa.h
    dtoa/strtod.h
    dtoa/utils.h

    text/AtomicString.h
    text/AtomicStringImpl.h
    text/AtomicStringTable.h
    text/Base64.h
    text/CString.h
    text/IntegerToStringConversion.h
    text/StringBuffer.h
    text/StringHash.h
    text/StringImpl.h
    text/WTFString.h

    threads/BinarySemaphore.h

    unicode/CharacterNames.h
    unicode/Collator.h
    unicode/UTF8.h
    unicode/Unicode.h
)

set(WTF_SOURCES
    ArrayBuffer.cpp
    ArrayBufferView.cpp
    Assertions.cpp
    Atomics.cpp
    BitVector.cpp
    CryptographicallyRandomNumber.cpp
    CurrentTime.cpp
    DateMath.cpp
    DataLog.cpp
    DecimalNumber.cpp
    DynamicAnnotations.cpp
    FastMalloc.cpp
    FilePrintStream.cpp
    FunctionDispatcher.cpp
    GregorianDateTime.cpp
    HashTable.cpp
    MD5.cpp
    MainThread.cpp
    MediaTime.cpp
    MetaAllocator.cpp
    NullPtr.cpp
    OSRandomSource.cpp
    NumberOfCores.cpp
    RAMSize.cpp
    PageAllocationAligned.cpp
    PageBlock.cpp
    ParallelJobsGeneric.cpp
    PrintStream.cpp
    RandomNumber.cpp
    RefCountedLeakCounter.cpp
    SHA1.cpp
    StackBounds.cpp
    StringPrintStream.cpp
    TCSystemAlloc.cpp
    Threading.cpp
    TypeTraits.cpp
    WTFThreadData.cpp
    dtoa.cpp

    dtoa/bignum-dtoa.cc
    dtoa/bignum.cc
    dtoa/cached-powers.cc
    dtoa/diy-fp.cc
    dtoa/double-conversion.cc
    dtoa/fast-dtoa.cc
    dtoa/fixed-dtoa.cc
    dtoa/strtod.cc

    text/AtomicString.cpp
    text/AtomicStringTable.cpp
    text/Base64.cpp
    text/CString.cpp
    text/StringBuilder.cpp
    text/StringImpl.cpp
    text/StringStatics.cpp
    text/WTFString.cpp

    threads/BinarySemaphore.cpp

    unicode/UTF8.cpp
)

set(WTF_INCLUDE_DIRECTORIES
    "${WTF_DIR}"
    "${WTF_DIR}/wtf"
    "${WTF_DIR}/wtf/dtoa"
    "${WTF_DIR}/wtf/threads"
    "${WTF_DIR}/wtf/unicode"
    "${THIRDPARTY_DIR}"
    "${CMAKE_BINARY_DIR}"
)

set(WTF_LIBRARIES
    ${CMAKE_DL_LIBS}
)

if (WTF_USE_ICU_UNICODE)
    list(APPEND WTF_HEADERS
        unicode/icu/UnicodeIcu.h
    )
    list(APPEND WTF_SOURCES
        unicode/icu/CollatorICU.cpp
    )
    list(APPEND WTF_INCLUDE_DIRECTORIES
        ${ICU_INCLUDE_DIRS}
    )
    list(APPEND WTF_LIBRARIES
        ${ICU_I18N_LIBRARIES}
        ${ICU_LIBRARIES}
    )
elseif (WTF_USE_WCHAR_UNICODE)
    list(APPEND WTF_HEADERS
        unicode/wchar/UnicodeWchar.h
    )
    list(APPEND WTF_SOURCES
        unicode/CollatorDefault.cpp
        unicode/wchar/UnicodeWchar.cpp
    )
endif ()

if (WIN32)
    list(APPEND WTF_SOURCES
        OSAllocatorWin.cpp
        ThreadSpecificWin.cpp
        ThreadingWin.cpp

        win/OwnPtrWin.cpp
    )
else ()
    list(APPEND WTF_HEADERS
        ThreadIdentifierDataPthreads.h
    )
    list(APPEND WTF_SOURCES
        OSAllocatorPosix.cpp
        ThreadIdentifierDataPthreads.cpp
        ThreadingPthreads.cpp
    )
endif ()

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

WEBKIT_WRAP_SOURCELIST(${WTF_SOURCES})
include_directories(${WTF_INCLUDE_DIRECTORIES})
add_definitions(-DBUILDING_WTF)
add_library(WTF STATIC ${WTF_HEADERS} ${WTF_SOURCES})
target_link_libraries(WTF ${WTF_LIBRARIES})
set_target_properties(WTF PROPERTIES FOLDER "JavaScriptCore")

if (WTF_OUTPUT_NAME)
    set_target_properties(WTF PROPERTIES OUTPUT_NAME ${WTF_OUTPUT_NAME})
endif ()
