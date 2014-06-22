list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/accessibility/atk"
    "${WEBCORE_DIR}/editing/atk"
    "${WEBCORE_DIR}/page/gtk"
    "${WEBCORE_DIR}/platform/cairo"
    "${WEBCORE_DIR}/platform/gtk"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/graphics/egl"
    "${WEBCORE_DIR}/platform/graphics/glx"
    "${WEBCORE_DIR}/platform/graphics/gtk"
    "${WEBCORE_DIR}/platform/graphics/freetype"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/ng"
    "${WEBCORE_DIR}/platform/graphics/opengl"
    "${WEBCORE_DIR}/platform/linux"
    "${WEBCORE_DIR}/platform/mediastream/gstreamer"
    "${WEBCORE_DIR}/platform/network/gtk"
    "${WEBCORE_DIR}/platform/network/soup"
    "${WEBCORE_DIR}/platform/text/gtk"
    "${WEBCORE_DIR}/plugins/gtk"
)

list(APPEND WebCore_SOURCES
    accessibility/atk/AccessibilityObjectAtk.cpp
    accessibility/atk/AXObjectCacheAtk.cpp
    accessibility/atk/WebKitAccessibleHyperlink.cpp
    accessibility/atk/WebKitAccessibleInterfaceAction.cpp
    accessibility/atk/WebKitAccessibleInterfaceComponent.cpp
    accessibility/atk/WebKitAccessibleInterfaceDocument.cpp
    accessibility/atk/WebKitAccessibleInterfaceEditableText.cpp
    accessibility/atk/WebKitAccessibleInterfaceHyperlinkImpl.cpp
    accessibility/atk/WebKitAccessibleInterfaceHypertext.cpp
    accessibility/atk/WebKitAccessibleInterfaceImage.cpp
    accessibility/atk/WebKitAccessibleInterfaceSelection.cpp
    accessibility/atk/WebKitAccessibleInterfaceTable.cpp
    accessibility/atk/WebKitAccessibleInterfaceText.cpp
    accessibility/atk/WebKitAccessibleInterfaceValue.cpp
    accessibility/atk/WebKitAccessibleUtil.cpp
    accessibility/atk/WebKitAccessibleWrapperAtk.cpp

    editing/SmartReplaceICU.cpp

    editing/atk/FrameSelectionAtk.cpp

    loader/soup/CachedRawResourceSoup.cpp
    loader/soup/SubresourceLoaderSoup.cpp

    page/gtk/DragControllerGtk.cpp
    page/gtk/EventHandlerGtk.cpp

    platform/cairo/WidgetBackingStoreCairo.cpp

    platform/Cursor.cpp

    platform/audio/gtk/AudioBusGtk.cpp
    platform/audio/gstreamer/AudioDestinationGStreamer.cpp
    platform/audio/gstreamer/AudioFileReaderGStreamer.cpp
    platform/audio/gstreamer/FFTFrameGStreamer.cpp
    platform/audio/gstreamer/WebKitWebAudioSourceGStreamer.cpp

    platform/gtk/AsyncFileSystemGtk.cpp
    platform/gtk/ClipboardGtk.cpp
    platform/gtk/ClipboardUtilitiesGtk.cpp
    platform/gtk/ContextMenuGtk.cpp
    platform/gtk/ContextMenuItemGtk.cpp
    platform/gtk/CursorGtk.cpp
    platform/gtk/DataObjectGtk.cpp
    platform/gtk/DragDataGtk.cpp
    platform/gtk/DragIcon.cpp
    platform/gtk/DragImageGtk.cpp
    platform/gtk/ErrorsGtk.cpp
    platform/gtk/EventLoopGtk.cpp
    platform/gtk/FileSystemGtk.cpp
    platform/gtk/GamepadsGtk.cpp
    platform/gtk/GOwnPtrGtk.cpp
    platform/gtk/GRefPtrGtk.cpp
    platform/gtk/GtkClickCounter.cpp
    platform/gtk/GtkDragAndDropHelper.cpp
    platform/gtk/GtkInputMethodFilter.cpp
    platform/gtk/GtkPluginWidget.cpp
    platform/gtk/GtkPopupMenu.cpp
    platform/gtk/GtkUtilities.cpp
    platform/gtk/GtkVersioning.c
    platform/gtk/KeyBindingTranslator.cpp
    platform/gtk/LanguageGtk.cpp
    platform/gtk/LocalizedStringsGtk.cpp
    platform/gtk/LoggingGtk.cpp
    platform/gtk/MainFrameScrollbarGtk.cpp
    platform/gtk/MIMETypeRegistryGtk.cpp
    platform/gtk/PasteboardGtk.cpp
    platform/gtk/PasteboardHelper.cpp
    platform/gtk/PlatformKeyboardEventGtk.cpp
    platform/gtk/PlatformMouseEventGtk.cpp
    platform/gtk/PlatformScreenGtk.cpp
    platform/gtk/PlatformWheelEventGtk.cpp
    platform/gtk/PopupMenuGtk.cpp
    platform/gtk/RedirectedXCompositeWindow.cpp
    platform/gtk/RenderThemeGtk2.cpp
    platform/gtk/RenderThemeGtk3.cpp
    platform/gtk/RenderThemeGtk.cpp
    platform/gtk/RunLoopGtk.cpp
    platform/gtk/ScrollbarThemeGtk2.cpp
    platform/gtk/ScrollbarThemeGtk3.cpp
    platform/gtk/ScrollbarThemeGtk.cpp
    platform/gtk/ScrollViewGtk.cpp
    platform/gtk/SearchPopupMenuGtk.cpp
    platform/gtk/SharedBufferGtk.cpp
    platform/gtk/SharedTimerGtk.cpp
    platform/gtk/SoundGtk.cpp
    platform/gtk/TemporaryLinkStubs.cpp
    platform/gtk/UserAgentGtk.cpp
    platform/gtk/WebKitAuthenticationWidget.cpp
    platform/gtk/WidgetBackingStoreGtkX11.cpp
    platform/gtk/WidgetGtk.cpp
    platform/gtk/WidgetRenderingContext.cpp

    platform/graphics/cairo/BitmapImageCairo.cpp
    platform/graphics/cairo/CairoUtilities.cpp
    platform/graphics/cairo/DrawingBufferCairo.cpp
    platform/graphics/cairo/FontCairo.cpp
    platform/graphics/cairo/FontCairoHarfbuzzNG.cpp
    platform/graphics/cairo/GLContext.cpp
    platform/graphics/cairo/GradientCairo.cpp
    platform/graphics/cairo/GraphicsContext3DCairo.cpp
    platform/graphics/cairo/GraphicsContext3DPrivate.cpp
    platform/graphics/cairo/GraphicsContextCairo.cpp
    platform/graphics/cairo/ImageBufferCairo.cpp
    platform/graphics/cairo/ImageCairo.cpp
    platform/graphics/cairo/IntRectCairo.cpp
    platform/graphics/cairo/OwnPtrCairo.cpp
    platform/graphics/cairo/PathCairo.cpp
    platform/graphics/cairo/PatternCairo.cpp
    platform/graphics/cairo/PlatformContextCairo.cpp
    platform/graphics/cairo/PlatformPathCairo.cpp
    platform/graphics/cairo/RefPtrCairo.cpp
    platform/graphics/cairo/TileCairo.cpp
    platform/graphics/cairo/TiledBackingStoreBackendCairo.cpp
    platform/graphics/cairo/TransformationMatrixCairo.cpp

    platform/graphics/egl/GLContextEGL.cpp

    platform/graphics/glx/GLContextGLX.cpp

    platform/graphics/gtk/ColorGtk.cpp
    platform/graphics/gtk/FullscreenVideoControllerGtk.cpp
    platform/graphics/gtk/GdkCairoUtilities.cpp
    platform/graphics/gtk/IconGtk.cpp
    platform/graphics/gtk/ImageBufferGtk.cpp
    platform/graphics/gtk/ImageGtk.cpp
    platform/graphics/gtk/IntPointGtk.cpp
    platform/graphics/gtk/IntRectGtk.cpp

    platform/graphics/freetype/FontCacheFreeType.cpp
    platform/graphics/freetype/FontCustomPlatformDataFreeType.cpp
    platform/graphics/freetype/FontPlatformDataFreeType.cpp
    platform/graphics/freetype/GlyphPageTreeNodeFreeType.cpp
    platform/graphics/freetype/SimpleFontDataFreeType.cpp

    platform/graphics/gstreamer/GRefPtrGStreamer.cpp
    platform/graphics/gstreamer/GStreamerGWorld.cpp
    platform/graphics/gstreamer/GStreamerUtilities.cpp
    platform/graphics/gstreamer/GStreamerVersioning.cpp
    platform/graphics/gstreamer/ImageGStreamerCairo.cpp
    platform/graphics/gstreamer/MediaPlayerPrivateGStreamerBase.cpp
    platform/graphics/gstreamer/MediaPlayerPrivateGStreamer.cpp
    platform/graphics/gstreamer/PlatformVideoWindowGtk.cpp
    platform/graphics/gstreamer/VideoSinkGStreamer.cpp
    platform/graphics/gstreamer/WebKitWebSourceGStreamer.cpp

    platform/graphics/harfbuzz/HarfBuzzFaceCairo.cpp
    platform/graphics/harfbuzz/HarfBuzzFace.cpp
    platform/graphics/harfbuzz/HarfBuzzShaper.cpp

    platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
    platform/graphics/opengl/Extensions3DOpenGL.cpp
    platform/graphics/opengl/Extensions3DOpenGLES.cpp
    platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
    platform/graphics/opengl/GraphicsContext3DOpenGL.cpp

    platform/graphics/OpenGLShims.cpp
    platform/graphics/WOFFFileFormat.cpp

    platform/image-decoders/cairo/ImageDecoderCairo.cpp

    platform/linux/GamepadDeviceLinux.cpp

    platform/mediastream/gstreamer/MediaStreamCenterGStreamer.cpp

    platform/network/gtk/CredentialBackingStore.cpp

    platform/network/soup/AuthenticationChallengeSoup.cpp
    platform/network/soup/CookieJarSoup.cpp
    platform/network/soup/CookieStorageSoup.cpp
    platform/network/soup/CredentialStorageSoup.cpp
    platform/network/soup/DNSSoup.cpp
    platform/network/soup/GOwnPtrSoup.cpp
    platform/network/soup/NetworkStorageSessionSoup.cpp
    platform/network/soup/ProxyResolverSoup.cpp
    platform/network/soup/ProxyServerSoup.cpp
    platform/network/soup/ResourceErrorSoup.cpp
    platform/network/soup/ResourceHandleSoup.cpp
    platform/network/soup/ResourceRequestSoup.cpp
    platform/network/soup/ResourceResponseSoup.cpp
    platform/network/soup/SocketStreamHandleSoup.cpp
    platform/network/soup/SoupURIUtils.cpp

    platform/PlatformStrategies.cpp

    platform/text/gtk/TextBreakIteratorInternalICUGtk.cpp
    platform/text/enchant/TextCheckerEnchant.cpp
    platform/text/LocaleICU.cpp
    platform/text/TextBreakIteratorICU.cpp
    platform/text/TextCodecICU.cpp
    platform/text/TextEncodingDetectorICU.cpp
)

if (ENABLE_NETSCAPE_PLUGIN_API)
    list(APPEND WebCore_SOURCES
        plugins/PluginDatabase.cpp
        plugins/PluginDebug.cpp
        plugins/PluginPackage.cpp plugins/PluginStream.cpp
        plugins/PluginView.cpp

        plugins/gtk/PluginPackageGtk.cpp
        plugins/gtk/PluginViewGtk.cpp
        plugins/gtk/gtk2xtbin.c
    )
else ()
    list(APPEND WebCore_SOURCES
        plugins/PluginPackageNone.cpp
        plugins/PluginViewNone.cpp
    )
endif ()

list(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/mediaControlsGtk.css
)

list(APPEND WebCore_LIBRARIES
    ${ATK_LIBRARIES}
    ${ENCHANT_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${ICU_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${PNG_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_GMODULE_LIBRARIES}
    ${GAIL3_LIBRARIES}
    ${GTK3_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    ${WEBP_LIBRARIES}
    ${XT_LIBRARIES}
    ${X11_X11_LIB}
    ${X11_Xcomposite_LIB}
    ${X11_Xrender_LIB}
    ${X11_Xdamage_LIB}
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    ${ATK_INCLUDE_DIRS}
    ${ENCHANT_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${ICU_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
    ${SQLITE_INCLUDE_DIR}
    ${GAIL3_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GTK3_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
    ${WEBP_INCLUDE_DIRS}
    ${XT_INCLUDE_DIRS}
)

if (ENABLE_VIDEO OR ENABLE_WEB_AUDIO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${WEBCORE_DIR}/platform/graphics/gstreamer
        ${GSTREAMER_INCLUDE_DIRS}
        ${GSTREAMER_BASE_INCLUDE_DIRS}
        ${GSTREAMER_APP_INCLUDE_DIRS}
        ${GSTREAMER_PBUTILS_INCLUDE_DIRS}
    )

    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_LIBRARIES}
        ${GSTREAMER_BASE_LIBRARIES}
        ${GSTREAMER_APP_LIBRARIES}
        ${GSTREAMER_PBUTILS_LIBRARIES}
    )
    # Avoiding a GLib deprecation warning due to GStreamer API using deprecated classes.
    set_source_files_properties(platform/audio/gstreamer/WebKitWebAudioSourceGStreamer.cpp PROPERTIES COMPILE_DEFINITIONS "GLIB_DISABLE_DEPRECATION_WARNINGS=1")
endif ()

if (ENABLE_VIDEO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_VIDEO_LIBRARIES}
    )
endif ()

if (ENABLE_WEB_AUDIO)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        ${WEBCORE_DIR}/platform/audio/gstreamer
        ${GSTREAMER_AUDIO_INCLUDE_DIRS}
        ${GSTREAMER_FFT_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${GSTREAMER_AUDIO_LIBRARIES}
        ${GSTREAMER_FFT_LIBRARIES}
    )
endif ()

if (ENABLE_TEXTURE_MAPPER)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/platform/graphics/texmap"
    )
    list(APPEND WebCore_SOURCES
        platform/graphics/texmap/GraphicsLayerTextureMapper.cpp
        platform/graphics/texmap/TextureMapperGL.cpp
        platform/graphics/texmap/TextureMapperShaderProgram.cpp
    )
endif ()

if (WTF_USE_EGL)
    list(APPEND WebCore_LIBRARIES
        ${EGL_LIBRARY}
    )
endif ()

install(FILES
            "${WEBCORE_DIR}/Resources/textAreaResizeCorner.png"
            "${WEBCORE_DIR}/Resources/nullPlugin.png"
            "${WEBCORE_DIR}/Resources/urlIcon.png"
            "${WEBCORE_DIR}/Resources/missingImage.png"
            "${WEBCORE_DIR}/Resources/panIcon.png"
            "${WEBCORE_DIR}/Resources/deleteButton.png"
            "${WEBCORE_DIR}/Resources/inputSpeech.png"
        DESTINATION
            "${DATA_INSTALL_DIR}/images")

if (ENABLE_WEB_AUDIO)
    install(FILES
                "${WEBCORE_DIR}/platform/audio/resources/Composite.wav"
            DESTINATION
                "${DATA_INSTALL_DIR}/resources/audio")
endif ()
