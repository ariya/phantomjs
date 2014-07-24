set(DumpRenderTree_SOURCES
    ${TOOLS_DIR}/DumpRenderTree/AccessibilityController.cpp
    ${TOOLS_DIR}/DumpRenderTree/AccessibilityTextMarker.cpp
    ${TOOLS_DIR}/DumpRenderTree/AccessibilityUIElement.cpp
    ${TOOLS_DIR}/DumpRenderTree/DumpRenderTreeCommon.cpp
    ${TOOLS_DIR}/DumpRenderTree/CyclicRedundancyCheck.cpp
    ${TOOLS_DIR}/DumpRenderTree/GCController.cpp
    ${TOOLS_DIR}/DumpRenderTree/TestRunner.cpp
    ${TOOLS_DIR}/DumpRenderTree/PixelDumpSupport.cpp
    ${TOOLS_DIR}/DumpRenderTree/WorkQueue.cpp
    ${TOOLS_DIR}/DumpRenderTree/atk/AccessibilityCallbacksAtk.cpp
    ${TOOLS_DIR}/DumpRenderTree/atk/AccessibilityControllerAtk.cpp
    ${TOOLS_DIR}/DumpRenderTree/atk/AccessibilityUIElementAtk.cpp
    ${TOOLS_DIR}/DumpRenderTree/cairo/PixelDumpSupportCairo.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/AccessibilityControllerEfl.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/AccessibilityUIElementEfl.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/DumpHistoryItem.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/DumpRenderTree.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/DumpRenderTreeChrome.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/DumpRenderTreeView.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/EditingCallbacks.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/EventSender.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/FontManagement.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/GCControllerEfl.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/JSStringUtils.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/TestRunnerEfl.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/PixelDumpSupportEfl.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/TextInputController.cpp
    ${TOOLS_DIR}/DumpRenderTree/efl/WorkQueueItemEfl.cpp
)

set(DumpRenderTree_LIBRARIES
    JavaScriptCore
    WebCoreTestSupport
    WebCore
    WebKit
    WTF
    ${CAIRO_LIBRARIES}
    ${ECORE_LIBRARIES}
    ${ECORE_EVAS_LIBRARIES}
    ${ECORE_FILE_LIBRARIES}
    ${ECORE_INPUT_LIBRARIES}
    ${EDJE_LIBRARIES}
    ${EINA_LIBRARIES}
    ${EO_LIBRARIES}
    ${EVAS_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES} -lm
    ${SQLITE_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
)

set(DumpRenderTree_INCLUDE_DIRECTORIES
    "${WEBKIT_DIR}/efl/ewk"
    ${WEBKIT_DIR}/efl
    ${WEBKIT_DIR}/efl/WebCoreSupport
    ${WEBCORE_DIR}
    ${WEBCORE_DIR}/bridge
    ${WEBCORE_DIR}/bridge/jsc
    ${WEBCORE_DIR}/bindings
    ${WEBCORE_DIR}/dom
    ${WEBCORE_DIR}/editing
    ${WEBCORE_DIR}/css
    ${WEBCORE_DIR}/html
    ${WEBCORE_DIR}/inspector
    ${WEBCORE_DIR}/page
    ${WEBCORE_DIR}/page/animation
    ${WEBCORE_DIR}/platform
    ${WEBCORE_DIR}/platform/animation
    ${WEBCORE_DIR}/platform/text
    ${WEBCORE_DIR}/platform/graphics
    ${WEBCORE_DIR}/platform/graphics/cairo
    ${WEBCORE_DIR}/platform/graphics/transforms
    ${WEBCORE_DIR}/platform/network
    ${WEBCORE_DIR}/platform/network/soup
    ${WEBCORE_DIR}/plugins
    ${WEBCORE_DIR}/rendering
    ${WEBCORE_DIR}/rendering/shapes
    ${WEBCORE_DIR}/rendering/style
    ${WEBCORE_DIR}/history
    ${WEBCORE_DIR}/loader
    ${WEBCORE_DIR}/loader/cache
    ${WEBCORE_DIR}/loader/icon
    ${JAVASCRIPTCORE_DIR}
    ${JAVASCRIPTCORE_DIR}/API
    ${JAVASCRIPTCORE_DIR}/assembler
    ${JAVASCRIPTCORE_DIR}/bytecode
    ${JAVASCRIPTCORE_DIR}/dfg
    ${JAVASCRIPTCORE_DIR}/disassembler
    ${JAVASCRIPTCORE_DIR}/heap
    ${JAVASCRIPTCORE_DIR}/interpreter
    ${JAVASCRIPTCORE_DIR}/jit
    ${JAVASCRIPTCORE_DIR}/llint
    ${JAVASCRIPTCORE_DIR}/profiler
    ${JAVASCRIPTCORE_DIR}/runtime
    ${JAVASCRIPTCORE_DIR}/ForwardingHeaders
    ${TOOLS_DIR}/DumpRenderTree
    ${TOOLS_DIR}/DumpRenderTree/atk
    ${TOOLS_DIR}/DumpRenderTree/cairo
    ${TOOLS_DIR}/DumpRenderTree/efl
    ${WTF_DIR}
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_BINARY_DIR}
    ${DERIVED_SOURCES_WEBCORE_DIR}
    ${WEBCORE_DIR}/bindings/js
    ${WEBCORE_DIR}/testing/js
    ${CAIRO_INCLUDE_DIRS}
    ${ECORE_INCLUDE_DIRS}
    ${ECORE_INCLUDE_DIRS}
    ${ECORE_EVAS_INCLUDE_DIRS}
    ${ECORE_FILE_INCLUDE_DIRS}
    ${ECORE_INPUT_INCLUDE_DIRS}
    ${EDJE_INCLUDE_DIRS}
    ${EINA_INCLUDE_DIRS}
    ${EO_INCLUDE_DIRS}
    ${EVAS_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIR}
    ${GLIB_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
)

if (ENABLE_ACCESSIBILITY)
    list(APPEND DumpRenderTree_INCLUDE_DIRECTORIES
        ${TOOLS_DIR}/DumpRenderTree/atk
        ${ATK_INCLUDE_DIRS}
    )
    list(APPEND DumpRenderTree_LIBRARIES
        ${ATK_LIBRARIES}
    )
endif ()

# FIXME: DOWNLOADED_FONTS_DIR should not hardcode the directory
# structure. See <https://bugs.webkit.org/show_bug.cgi?id=81475>.
add_definitions(-DFONTS_CONF_DIR="${TOOLS_DIR}/DumpRenderTree/gtk/fonts"
                -DDOWNLOADED_FONTS_DIR="${CMAKE_SOURCE_DIR}/WebKitBuild/Dependencies/Source/webkitgtk-test-fonts-0.0.3")

include_directories(${DumpRenderTree_INCLUDE_DIRECTORIES})

add_executable(DumpRenderTree ${DumpRenderTree_SOURCES})
target_link_libraries(DumpRenderTree ${DumpRenderTree_LIBRARIES})
set_target_properties(DumpRenderTree PROPERTIES FOLDER "Tools")
