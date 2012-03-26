LIST(APPEND WTF_HEADERS
    unicode/wince/UnicodeWinCE.h

    ${3RDPARTY_DIR}/ce-compat/ce_time.h
    ${3RDPARTY_DIR}/ce-compat/ce_unicode.h
)

LIST(APPEND WTF_SOURCES
    NullPtr.cpp
    OSAllocatorWin.cpp
    TCSystemAlloc.cpp
    ThreadingWin.cpp
    ThreadSpecificWin.cpp

    unicode/CollatorDefault.cpp
    unicode/wince/UnicodeWinCE.cpp

    win/MainThreadWin.cpp
    win/OwnPtrWin.cpp

    ${3RDPARTY_DIR}/ce-compat/ce_time.c
    ${3RDPARTY_DIR}/ce-compat/ce_unicode.cpp
)

LIST(APPEND WTF_LIBRARIES
    mmtimer
)
