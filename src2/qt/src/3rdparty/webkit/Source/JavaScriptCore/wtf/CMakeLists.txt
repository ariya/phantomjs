SET(WTF_HEADERS
    ASCIICType.h
    AVLTree.h
    Alignment.h
    AlwaysInline.h
    Assertions.h
    Atomics.h
    Bitmap.h
    BumpPointerAllocator.h
    ByteArray.h
    Complex.h
    CrossThreadRefCounted.h
    CryptographicallyRandomNumber.h
    CurrentTime.h
    DateMath.h
    DecimalNumber.h
    Decoder.h
    Deque.h
    DisallowCType.h
    DoublyLinkedList.h
    DynamicAnnotations.h
    Encoder.h
    FastAllocBase.h
    FastMalloc.h
    FixedArray.h
    Forward.h
    GetPtr.h
    HashCountedSet.h
    HashFunctions.h
    HashIterators.h
    HashMap.h
    HashSet.h
    HashTable.h
    HashTraits.h
    HexNumber.h
    ListHashSet.h
    ListRefPtr.h
    Locker.h
    MD5.h
    MainThread.h
    MallocZoneSupport.h
    MathExtras.h
    MessageQueue.h
    NonCopyingSort.h
    Noncopyable.h
    NotFound.h
    NullPtr.h
    OSAllocator.h
    OSRandomSource.h
    OwnArrayPtr.h
    OwnFastMallocPtr.h
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
    RandomNumber.h
    RandomNumberSeed.h
    RefCounted.h
    RefCountedLeakCounter.h
    RefPtr.h
    RefPtrHashMap.h
    RetainPtr.h
    SegmentedVector.h
    SHA1.h
    StackBounds.h
    StaticConstructors.h
    StdLibExtras.h
    StringExtras.h
    StringHasher.h
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
    UnusedParam.h
    VMTags.h
    ValueCheck.h
    Vector.h
    VectorTraits.h
    WTFThreadData.h
    dtoa.h

    text/AtomicString.h
    text/AtomicStringImpl.h
    text/CString.h
    text/StringBuffer.h
    text/StringHash.h
    text/StringImpl.h
    text/StringImplBase.h
    text/WTFString.h

    unicode/CharacterNames.h
    unicode/Collator.h
    unicode/UTF8.h
    unicode/Unicode.h
)

SET(WTF_SOURCES
    Assertions.cpp
    ByteArray.cpp
    CryptographicallyRandomNumber.cpp
    CurrentTime.cpp
    DecimalNumber.cpp
    DynamicAnnotations.cpp
    FastMalloc.cpp
    HashTable.cpp
    MainThread.cpp
    MD5.cpp
    OSRandomSource.cpp
    ParallelJobsGeneric.cpp
    RandomNumber.cpp
    RefCountedLeakCounter.cpp
    SHA1.cpp
    StackBounds.cpp
    StringExtras.cpp
    Threading.cpp
    TypeTraits.cpp
    WTFThreadData.cpp
    dtoa.cpp

    text/AtomicString.cpp
    text/CString.cpp
    text/StringBuilder.cpp
    text/StringImpl.cpp
    text/StringStatics.cpp
    text/WTFString.cpp

    unicode/UTF8.cpp
)

SET(WTF_LIBRARIES
)

SET(WTF_PORT_FLAGS )
INCLUDE_IF_EXISTS(${JAVASCRIPTCORE_DIR}/wtf/CMakeLists${PORT}.txt)

LIST(APPEND WTF_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_SOURCE_DIR}/ThirdParty"
)

WEBKIT_WRAP_SOURCELIST(${WTF_SOURCES})
INCLUDE_DIRECTORIES(${WTF_INCLUDE_DIRECTORIES})
ADD_DEFINITIONS(-DBUILDING_WTF)
ADD_LIBRARY(${WTF_LIBRARY_NAME} ${WTF_LIBRARY_TYPE} ${WTF_HEADERS} ${WTF_SOURCES})
TARGET_LINK_LIBRARIES(${WTF_LIBRARY_NAME} ${WTF_LIBRARIES})

IF (WTF_LINK_FLAGS)
    ADD_TARGET_PROPERTIES(${WTF_LIBRARY_NAME} LINK_FLAGS "${WTF_LINK_FLAGS}")
ENDIF ()

IF (SHARED_CORE)
    SET_TARGET_PROPERTIES(${WTF_LIBRARY_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
    INSTALL(TARGETS ${WTF_LIBRARY_NAME} DESTINATION lib)
ENDIF ()
