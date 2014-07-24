set(JSC_SOURCES
    ../jsc.cpp
)

set(JSC_LIBRARIES
    WTF
    JavaScriptCore
    ${CMAKE_DL_LIBS}
)

if ("${JavaScriptCore_LIBRARY_TYPE}" MATCHES "STATIC")
    add_definitions(-DSTATICALLY_LINKED_WITH_JavaScriptCore)
endif ()

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

WEBKIT_WRAP_SOURCELIST(${JSC_SOURCES})
include_directories(./ ${JavaScriptCore_INCLUDE_DIRECTORIES})
add_executable(jsc ${JSC_SOURCES})
target_link_libraries(jsc ${JSC_LIBRARIES})
set_target_properties(jsc PROPERTIES FOLDER "JavaScriptCore")

if (JSC_OUTPUT_NAME)
    set_target_properties(jsc PROPERTIES OUTPUT_NAME ${JSC_OUTPUT_NAME})
endif ()

if (JSC_LINK_FLAGS)
    ADD_TARGET_PROPERTIES(jsc LINK_FLAGS "${JSC_LINK_FLAGS}")
endif ()

if (SHOULD_INSTALL_JS_SHELL)
    install(TARGETS jsc DESTINATION "${EXEC_INSTALL_DIR}")
endif ()
