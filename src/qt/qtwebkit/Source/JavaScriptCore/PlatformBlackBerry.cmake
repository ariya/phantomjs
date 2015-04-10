list(REMOVE_ITEM JavaScriptCore_SOURCES
    runtime/GCActivityCallback.cpp
)

list(APPEND JavaScriptCore_SOURCES
    runtime/GCActivityCallbackBlackBerry.cpp
)

install(FILES "wtf/Forward.h" DESTINATION usr/include/browser/webkit/wtf)
