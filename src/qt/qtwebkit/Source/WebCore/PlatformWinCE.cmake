list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/accessibility/win"
    "${WEBCORE_DIR}/platform/wince"
    "${WEBCORE_DIR}/platform/win"
    "${WEBCORE_DIR}/platform/graphics/wince"
    "${WEBCORE_DIR}/platform/graphics/win"
    "${WEBCORE_DIR}/platform/network/win"
    "${WEBCORE_DIR}/plugins/win"
    "${WEBCORE_DIR}/page/wince"
    "${WEBCORE_DIR}/page/win"
    "${3RDPARTY_DIR}/libjpeg"
    "${3RDPARTY_DIR}/libpng"
    "${3RDPARTY_DIR}/libxml2/include"
    "${3RDPARTY_DIR}/libxslt/include"
    "${3RDPARTY_DIR}/sqlite"
    "${3RDPARTY_DIR}/zlib"
)

list(APPEND WebCore_SOURCES
    accessibility/win/AccessibilityObjectWin.cpp

    html/HTMLSelectElementWin.cpp

    page/win/DragControllerWin.cpp
    page/win/EventHandlerWin.cpp
    page/wince/FrameWinCE.cpp

    rendering/RenderThemeWince.cpp

    plugins/win/PluginDatabaseWin.cpp

    platform/Cursor.cpp
    platform/LocalizedStrings.cpp
    platform/ScrollAnimatorNone.cpp

    platform/win/BitmapInfo.cpp
    platform/win/ClipboardUtilitiesWin.cpp
    platform/win/ClipboardWin.cpp
    platform/win/ContextMenuItemWin.cpp
    platform/win/ContextMenuWin.cpp
    platform/win/CursorWin.cpp
    platform/win/DragDataWin.cpp
    platform/win/DragImageWin.cpp
    platform/win/EditorWin.cpp
    platform/win/EventLoopWin.cpp
    platform/win/FileSystemWin.cpp
    platform/win/KeyEventWin.cpp
    platform/win/LanguageWin.cpp
    platform/win/LocalizedStringsWin.cpp
    platform/win/MIMETypeRegistryWin.cpp
    platform/win/PasteboardWin.cpp
    platform/win/PopupMenuWin.cpp
    platform/win/PlatformMouseEventWin.cpp
    platform/win/PlatformScreenWin.cpp
    platform/win/RunLoopWin.cpp
    platform/win/SSLKeyGeneratorWin.cpp
    platform/win/ScrollbarThemeWin.cpp
    platform/win/SearchPopupMenuWin.cpp
    platform/win/SharedBufferWin.cpp
    platform/win/SharedTimerWin.cpp
    platform/win/SoundWin.cpp
    platform/win/SystemInfo.cpp
    platform/win/WCDataObject.cpp
    platform/win/WebCoreInstanceHandle.cpp
    platform/win/WidgetWin.cpp
    platform/win/WheelEventWin.cpp

    platform/network/NetworkStorageSessionStub.cpp

    platform/network/win/CredentialStorageWin.cpp
    platform/network/win/CookieJarWin.cpp
    platform/network/win/NetworkStateNotifierWin.cpp
    platform/network/win/ProxyServerWin.cpp
    platform/network/win/ResourceHandleWin.cpp
    platform/network/win/SocketStreamHandleWin.cpp

    platform/graphics/opentype/OpenTypeUtilities.cpp

    platform/graphics/win/DIBPixelData.cpp
    platform/graphics/win/GDIExtras.cpp
    platform/graphics/win/IconWin.cpp
    platform/graphics/win/ImageWin.cpp
    platform/graphics/win/IntPointWin.cpp
    platform/graphics/win/IntRectWin.cpp
    platform/graphics/win/IntSizeWin.cpp

    platform/graphics/wince/FontCacheWince.cpp
    platform/graphics/wince/FontCustomPlatformData.cpp
    platform/graphics/wince/FontPlatformData.cpp
    platform/graphics/wince/FontWince.cpp
    platform/graphics/wince/GlyphPageTreeNodeWince.cpp
    platform/graphics/wince/GradientWince.cpp
    platform/graphics/wince/GraphicsContextWince.cpp
    platform/graphics/wince/ImageBufferWince.cpp
    platform/graphics/wince/ImageWinCE.cpp
    platform/graphics/wince/PathWince.cpp
    platform/graphics/wince/PlatformPathWince.cpp
    platform/graphics/wince/SharedBitmap.cpp
    platform/graphics/wince/SimpleFontDataWince.cpp

    platform/text/LocaleNone.cpp

    platform/text/win/TextCodecWin.cpp
)

list(APPEND WebCore_LIBRARIES
    libjpeg
    libpng
    libxml2
    libxslt
    sqlite
    crypt32
    iphlpapi
    wininet
)

if (ENABLE_NETSCAPE_PLUGIN_API)
    list(APPEND WebCore_SOURCES
        plugins/win/PluginMessageThrottlerWin.cpp
        plugins/win/PluginPackageWin.cpp
        plugins/win/PluginViewWin.cpp
    )
endif ()
