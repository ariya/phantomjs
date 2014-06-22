list(APPEND WTF_SOURCES
    threads/win/BinarySemaphoreWin.cpp

    win/MainThreadWin.cpp
)

list(APPEND WTF_LIBRARIES
    mmtimer
)
