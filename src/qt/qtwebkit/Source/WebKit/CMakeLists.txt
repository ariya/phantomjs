set(WebKit_INCLUDE_DIRECTORIES
    "${CMAKE_SOURCE_DIR}/Source"
    "${JAVASCRIPTCORE_DIR}/ForwardingHeaders"
    "${WEBKIT_DIR}"
    "${WEBCORE_DIR}"
    "${WEBCORE_DIR}/Modules/filesystem"
    "${WEBCORE_DIR}/Modules/networkinfo"
    "${WEBCORE_DIR}/Modules/webdatabase"
    "${WEBCORE_DIR}/accessibility"
    "${WEBCORE_DIR}/bindings/generic"
    "${WEBCORE_DIR}/bindings/js"
    "${WEBCORE_DIR}/bindings"
    "${WEBCORE_DIR}/bridge"
    "${WEBCORE_DIR}/bridge/c"
    "${WEBCORE_DIR}/bridge/jsc"
    "${WEBCORE_DIR}/css"
    "${WEBCORE_DIR}/dom"
    "${WEBCORE_DIR}/dom/default"
    "${WEBCORE_DIR}/editing"
    "${WEBCORE_DIR}/history"
    "${WEBCORE_DIR}/html"
    "${WEBCORE_DIR}/html/forms"
    "${WEBCORE_DIR}/html/shadow"
    "${WEBCORE_DIR}/inspector"
    "${WEBCORE_DIR}/loader"
    "${WEBCORE_DIR}/loader/appcache"
    "${WEBCORE_DIR}/loader/icon"
    "${WEBCORE_DIR}/loader/cache"
    "${WEBCORE_DIR}/page"
    "${WEBCORE_DIR}/page/animation"
    "${WEBCORE_DIR}/platform"
    "${WEBCORE_DIR}/platform/animation"
    "${WEBCORE_DIR}/platform/graphics"
    "${WEBCORE_DIR}/platform/graphics/filters"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/ng"
    "${WEBCORE_DIR}/platform/graphics/opengl"
    "${WEBCORE_DIR}/platform/graphics/transforms"
    "${WEBCORE_DIR}/platform/network"
    "${WEBCORE_DIR}/platform/sql"
    "${WEBCORE_DIR}/platform/text"
    "${WEBCORE_DIR}/plugins"
    "${WEBCORE_DIR}/rendering"
    "${WEBCORE_DIR}/rendering/shapes"
    "${WEBCORE_DIR}/rendering/style"
    "${WEBCORE_DIR}/storage"
    "${WEBCORE_DIR}/svg"
    "${WEBCORE_DIR}/svg/graphics"
    "${WEBCORE_DIR}/svg/properties"
    "${WEBCORE_DIR}/workers"
    "${JAVASCRIPTCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}/ForwardingHeaders"
    "${JAVASCRIPTCORE_DIR}/API"
    "${JAVASCRIPTCORE_DIR}/assembler"
    "${JAVASCRIPTCORE_DIR}/bytecode"
    "${JAVASCRIPTCORE_DIR}/bytecompiler"
    "${JAVASCRIPTCORE_DIR}/disassembler"
    "${JAVASCRIPTCORE_DIR}/dfg"
    "${JAVASCRIPTCORE_DIR}/heap"
    "${JAVASCRIPTCORE_DIR}/debugger"
    "${JAVASCRIPTCORE_DIR}/interpreter"
    "${JAVASCRIPTCORE_DIR}/jit"
    "${JAVASCRIPTCORE_DIR}/llint"
    "${JAVASCRIPTCORE_DIR}/parser"
    "${JAVASCRIPTCORE_DIR}/profiler"
    "${JAVASCRIPTCORE_DIR}/runtime"
    "${WTF_DIR}"
    "${DERIVED_SOURCES_WEBCORE_DIR}"
    "${CMAKE_BINARY_DIR}"
)

if (WTF_USE_SOUP)
  list(APPEND WebKit_INCLUDE_DIRECTORIES "${WEBCORE_DIR}/platform/network/soup")
endif ()

if (WTF_USE_CURL)
  list(APPEND WebKit_INCLUDE_DIRECTORIES "${WEBCORE_DIR}/platform/network/curl")
endif ()

set(WebKit_LIBRARIES
    JavaScriptCore
    WTF
    WebCore
)

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

add_definitions(-DBUILDING_WebKit)
include_directories(${WebKit_INCLUDE_DIRECTORIES})
add_library(WebKit ${WebKit_LIBRARY_TYPE} ${WebKit_SOURCES})
add_dependencies(WebKit WebCore)
target_link_libraries(WebKit ${WebKit_LIBRARIES})
set_target_properties(WebKit PROPERTIES FOLDER "WebKit")
set_target_properties(WebKit PROPERTIES LINK_INTERFACE_LIBRARIES "")

if (WebKit_LINK_FLAGS)
    ADD_TARGET_PROPERTIES(WebKit LINK_FLAGS "${WebKit_LINK_FLAGS}")
endif ()

if (VERSION_SCRIPT)
    ADD_TARGET_PROPERTIES(WebKit LINK_FLAGS "${VERSION_SCRIPT}")
endif ()

if (WebKit_OUTPUT_NAME)
    set_target_properties(WebKit PROPERTIES OUTPUT_NAME ${WebKit_OUTPUT_NAME})
endif ()

set_target_properties(WebKit PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
install(TARGETS WebKit DESTINATION "${LIB_INSTALL_DIR}")
