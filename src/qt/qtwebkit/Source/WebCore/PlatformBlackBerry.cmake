list(INSERT WebCore_INCLUDE_DIRECTORIES 0
    "${BLACKBERRY_THIRD_PARTY_DIR}" # For <unicode.h>, which is included from <sys/keycodes.h>.
)

list(REMOVE_ITEM WebCore_SOURCES
    html/shadow/MediaControlsApple.cpp
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/blackberry/CookieDatabaseBackingStore"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz"
    "${WEBCORE_DIR}/platform/graphics/opentype/"
    "${WEBCORE_DIR}/platform/network/blackberry"
    "${WEBCORE_DIR}/platform/network/blackberry/rss"
)

# Other sources
list(APPEND WebCore_SOURCES
    platform/blackberry/CookieDatabaseBackingStore/CookieDatabaseBackingStore.cpp
    platform/blackberry/AuthenticationChallengeManager.cpp
    platform/blackberry/CookieManager.cpp
    platform/blackberry/CookieMap.cpp
    platform/blackberry/CookieParser.cpp
    platform/blackberry/FileSystemBlackBerry.cpp
    platform/blackberry/ParsedCookie.cpp
    platform/graphics/WOFFFileFormat.cpp
    platform/graphics/opentype/OpenTypeSanitizer.cpp
    platform/image-encoders/JPEGImageEncoder.cpp
    platform/posix/FileSystemPOSIX.cpp
    platform/posix/SharedBufferPOSIX.cpp
    platform/text/LocaleNone.cpp
    platform/text/blackberry/TextBreakIteratorInternalICUBlackBerry.cpp
)

# Networking sources
list(APPEND WebCore_SOURCES
    platform/network/MIMESniffing.cpp
    platform/network/NetworkStorageSessionStub.cpp
    platform/network/ProxyServer.cpp
    platform/network/blackberry/AutofillBackingStore.cpp
    platform/network/blackberry/BlobStream.cpp
    platform/network/blackberry/CookieJarBlackBerry.cpp
    platform/network/blackberry/DNSBlackBerry.cpp
    platform/network/blackberry/DeferredData.cpp
    platform/network/blackberry/NetworkJob.cpp
    platform/network/blackberry/NetworkManager.cpp
    platform/network/blackberry/NetworkStateNotifierBlackBerry.cpp
    platform/network/blackberry/ProxyServerBlackBerry.cpp
    platform/network/blackberry/ResourceErrorBlackBerry.cpp
    platform/network/blackberry/ResourceHandleBlackBerry.cpp
    platform/network/blackberry/ResourceRequestBlackBerry.cpp
    platform/network/blackberry/ResourceResponseBlackBerry.cpp
    platform/network/blackberry/SocketStreamHandleBlackBerry.cpp
    platform/network/blackberry/rss/RSSAtomParser.cpp
    platform/network/blackberry/rss/RSS10Parser.cpp
    platform/network/blackberry/rss/RSS20Parser.cpp
    platform/network/blackberry/rss/RSSFilterStream.cpp
    platform/network/blackberry/rss/RSSGenerator.cpp
    platform/network/blackberry/rss/RSSParserBase.cpp
)

list(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/mediaControlsBlackBerry.css
    ${WEBCORE_DIR}/css/mediaControlsBlackBerryFullscreen.css
    ${WEBCORE_DIR}/css/themeBlackBerry.css
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/bridge/blackberry"
    "${WEBCORE_DIR}/history/blackberry"
    "${WEBCORE_DIR}/page/blackberry"
    "${WEBCORE_DIR}/page/scrolling/blackberry"
    "${WEBCORE_DIR}/platform/blackberry"
    "${WEBCORE_DIR}/platform/graphics/blackberry"
    "${WEBCORE_DIR}/platform/image-decoders/bmp"
    "${WEBCORE_DIR}/platform/image-decoders/gif"
    "${WEBCORE_DIR}/platform/image-decoders/ico"
    "${WEBCORE_DIR}/platform/image-decoders/jpeg"
    "${WEBCORE_DIR}/platform/image-decoders/png"
    "${WEBCORE_DIR}/platform/image-encoders"
    "${WEBCORE_DIR}/platform/network/blackberry"
    "${WEBCORE_DIR}/platform/text/blackberry"
    "${WEBKIT_DIR}/blackberry/Api"
    "${WEBKIT_DIR}/blackberry/WebCoreSupport"
    "${WEBKIT_DIR}/blackberry/WebKitSupport"
)

# BlackBerry sources
list(APPEND WebCore_SOURCES
    editing/blackberry/SmartReplaceBlackBerry.cpp
    html/shadow/MediaControlsBlackBerry.cpp
    page/blackberry/AccessibilityObjectBlackBerry.cpp
    page/blackberry/DragControllerBlackBerry.cpp
    page/blackberry/EventHandlerBlackBerry.cpp
    page/blackberry/SettingsBlackBerry.cpp
    platform/blackberry/CursorBlackBerry.cpp
    platform/blackberry/DragDataBlackBerry.cpp
    platform/blackberry/DragImageBlackBerry.cpp
    platform/blackberry/EventLoopBlackBerry.cpp
    platform/blackberry/LocalizedStringsBlackBerry.cpp
    platform/blackberry/LoggingBlackBerry.cpp
    platform/blackberry/MIMETypeRegistryBlackBerry.cpp
    platform/blackberry/PasteboardBlackBerry.cpp
    platform/blackberry/PlatformKeyboardEventBlackBerry.cpp
    platform/blackberry/PlatformMouseEventBlackBerry.cpp
    platform/blackberry/PlatformScreenBlackBerry.cpp
    platform/blackberry/PlatformTouchEventBlackBerry.cpp
    platform/blackberry/PlatformTouchPointBlackBerry.cpp
    platform/blackberry/PopupMenuBlackBerry.cpp
    platform/blackberry/RenderThemeBlackBerry.cpp
    platform/blackberry/RunLoopBlackBerry.cpp
    platform/blackberry/SSLKeyGeneratorBlackBerry.cpp
    platform/blackberry/ScrollbarThemeBlackBerry.cpp
    platform/blackberry/SearchPopupMenuBlackBerry.cpp
    platform/blackberry/SharedTimerBlackBerry.cpp
    platform/blackberry/SoundBlackBerry.cpp
    platform/blackberry/TemporaryLinkStubs.cpp
    platform/blackberry/WidgetBlackBerry.cpp
    platform/graphics/blackberry/FloatPointBlackBerry.cpp
    platform/graphics/blackberry/FloatRectBlackBerry.cpp
    platform/graphics/blackberry/FloatSizeBlackBerry.cpp
    platform/graphics/blackberry/IconBlackBerry.cpp
    platform/graphics/blackberry/ImageBlackBerry.cpp
    platform/graphics/blackberry/IntPointBlackBerry.cpp
    platform/graphics/blackberry/IntRectBlackBerry.cpp
    platform/graphics/blackberry/IntSizeBlackBerry.cpp
    platform/graphics/blackberry/MediaPlayerPrivateBlackBerry.cpp
    platform/text/blackberry/StringBlackBerry.cpp
)

# Credential Persistence sources
list(APPEND WebCore_SOURCES
    platform/network/blackberry/CredentialBackingStore.cpp
    platform/network/blackberry/CredentialStorageBlackBerry.cpp
)

# File System support
if (ENABLE_FILE_SYSTEM)
    list(APPEND WebCore_SOURCES
        platform/blackberry/AsyncFileSystemBlackBerry.cpp
    )
endif ()

if (ENABLE_SMOOTH_SCROLLING)
    list(APPEND WebCore_SOURCES
        platform/blackberry/ScrollAnimatorBlackBerry.cpp
    )
endif ()

if (ENABLE_REQUEST_ANIMATION_FRAME)
    list(APPEND WebCore_SOURCES
        platform/graphics/blackberry/DisplayRefreshMonitorBlackBerry.cpp
        platform/graphics/DisplayRefreshMonitor.cpp
    )
endif ()

if (ENABLE_WEBGL)
    add_definitions(-DWTF_USE_OPENGL_ES_2=1)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/gpu"
        "${WEBCORE_DIR}/platform/graphics/opengl"
    )
    list(APPEND WebCore_SOURCES
        platform/graphics/blackberry/DrawingBufferBlackBerry.cpp
        platform/graphics/blackberry/GraphicsContext3DBlackBerry.cpp
        platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
        platform/graphics/opengl/GraphicsContext3DOpenGLES.cpp
        platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
        platform/graphics/opengl/Extensions3DOpenGLES.cpp
        platform/graphics/gpu/SharedGraphicsContext3D.cpp
    )
endif ()

if (ENABLE_MEDIA_STREAM)
    list(APPEND WebCore_SOURCES
        platform/mediastream/blackberry/MediaStreamCenterBlackBerry.cpp
    )
endif ()

if (ENABLE_NETSCAPE_PLUGIN_API)
    list(APPEND WebCore_SOURCES
        plugins/blackberry/NPCallbacksBlackBerry.cpp
        plugins/blackberry/PluginPackageBlackBerry.cpp
        plugins/blackberry/PluginViewBlackBerry.cpp
        plugins/blackberry/PluginViewPrivateBlackBerry.cpp
    )
else ()
    list(APPEND WebCore_SOURCES
        plugins/PluginDataNone.cpp
    )
endif ()

if (ENABLE_TEXT_AUTOSIZING)
    list(APPEND WebCore_SOURCES
        ${WEBCORE_DIR}/rendering/TextAutosizer.cpp
    )
endif ()

# To speed up linking when working on accel comp, you can move this whole chunk
# to Source/WebKit/blackberry/CMakeListsBlackBerry.txt.
# Append to WebKit_SOURCES instead of WebCore_SOURCES.
if (WTF_USE_ACCELERATED_COMPOSITING)
    list(APPEND WebCore_SOURCES
        ${WEBCORE_DIR}/platform/graphics/GraphicsLayer.cpp
        ${WEBCORE_DIR}/platform/graphics/GraphicsLayerUpdater.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/CanvasLayerWebKitThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/EGLImageLayerWebKitThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/EGLImageLayerCompositingThreadClient.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/GraphicsLayerBlackBerry.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerAnimation.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerCompositingThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerFilterRenderer.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerRenderer.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerRendererSurface.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerTexture.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerTile.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerTiler.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/LayerWebKitThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/PluginLayerWebKitThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/TextureCacheCompositingThread.cpp
        ${WEBCORE_DIR}/platform/graphics/blackberry/WebGLLayerWebKitThread.cpp
        ${WEBCORE_DIR}/rendering/RenderLayerBacking.cpp
        ${WEBCORE_DIR}/rendering/RenderLayerCompositor.cpp
    )
endif ()

set(ENV{WEBKITDIR} ${CMAKE_SOURCE_DIR}/Source)
set(ENV{PLATFORMNAME} ${CMAKE_SYSTEM_NAME})
execute_process(
    COMMAND hostname
    OUTPUT_VARIABLE host
)
string(REPLACE "\n" "" host1 "${host}")
set(ENV{COMPUTERNAME} ${host1})

if ($ENV{PUBLIC_BUILD})
    add_definitions(-DPUBLIC_BUILD=$ENV{PUBLIC_BUILD})
endif ()

# Generate contents for PopupPicker.cpp
set(WebCore_POPUP_CSS_AND_JS
    ${WEBCORE_DIR}/Resources/blackberry/popupControlBlackBerry.css
    ${WEBCORE_DIR}/Resources/blackberry/selectControlBlackBerry.css
    ${WEBCORE_DIR}/Resources/blackberry/selectControlBlackBerry.js
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/PopupPicker.h ${DERIVED_SOURCES_WEBCORE_DIR}/PopupPicker.cpp
    MAIN_DEPENDENCY ${WEBCORE_DIR}/make-file-arrays.py
    DEPENDS ${WebCore_POPUP_CSS_AND_JS}
    COMMAND ${PYTHON_EXECUTABLE} ${WEBCORE_DIR}/make-file-arrays.py --out-h=${DERIVED_SOURCES_WEBCORE_DIR}/PopupPicker.h --out-cpp=${DERIVED_SOURCES_WEBCORE_DIR}/PopupPicker.cpp ${WebCore_POPUP_CSS_AND_JS}
)
list(APPEND WebCore_SOURCES ${DERIVED_SOURCES_WEBCORE_DIR}/PopupPicker.cpp)

set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> <LINK_FLAGS> cruT <TARGET> <OBJECTS>")
set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> <LINK_FLAGS> ruT <TARGET> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> <LINK_FLAGS> cruT <TARGET> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> <LINK_FLAGS> ruT <TARGET> <OBJECTS>")
