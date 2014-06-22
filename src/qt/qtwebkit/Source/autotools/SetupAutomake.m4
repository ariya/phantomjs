#######################################################################################
#                                     ATTENTION                                       #
# Please refer to the following guidelines if you're about to add an Automake         #
# conditional that reflects any configuration option you've probably also added.      #
# Chances are neither of these changes are required.                                  #
# http://trac.webkit.org/wiki/AddingFeatures#ActivatingafeatureforAutotoolsbasedports #
#######################################################################################

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_PROG_CC_C_O
AM_CONDITIONAL([GTK_API_VERSION_2],[test "$GTK_API_VERSION" = "2.0"])

# For the moment we need to check whether or not indexed database is
# enabled because it determines if we build leveldb or not.
AM_WEBKIT_FEATURE_CONDITIONAL([ENABLE_INDEXED_DATABASE])

# OS conditionals.
AM_CONDITIONAL([OS_WIN32],[test "$os_win32" = "yes"])
AM_CONDITIONAL([OS_UNIX],[test "$os_win32" = "no"])
AM_CONDITIONAL([OS_LINUX],[test "$os_linux" = "yes"])
AM_CONDITIONAL([OS_GNU],[test "$os_gnu" = "yes"])
AM_CONDITIONAL([OS_DARWIN],[test "$os_darwin" = "yes"])
AM_CONDITIONAL([OS_FREEBSD],[test "$os_freebsd" = "yes"])

# Target conditionals.
AM_CONDITIONAL([TARGET_X11], [test "$with_target" = "x11"])
AM_CONDITIONAL([TARGET_WIN32], [test "$with_target" = "win32"])
AM_CONDITIONAL([TARGET_QUARTZ], [test "$with_target" = "quartz"])
AM_CONDITIONAL([TARGET_DIRECTFB], [test "$with_target" = "directfb"])

# GStreamer feature conditionals.
AM_CONDITIONAL([USE_GSTREAMER], [test "$enable_video" = "yes" || test "$enable_web_audio" = "yes"])
AM_CONDITIONAL([USE_WEBAUDIO_GSTREAMER], [test "$enable_web_audio" = "yes"])

# ATSPI2 conditional.
AM_CONDITIONAL([HAVE_ATSPI2], [test "$have_atspi2" = "yes"])

# Accelerated compositing conditionals.
AM_CONDITIONAL([USE_ACCELERATED_COMPOSITING], [test "$enable_accelerated_compositing" = "yes"])
AM_CONDITIONAL([USE_TEXTURE_MAPPER_GL],
    [test "$enable_accelerated_compositing" = "yes" && test "$found_opengl" = "yes"])

# These are the same for now, but they will soon be separate.
AM_CONDITIONAL([USE_OPENGL], [test "$found_opengl" = "yes"])
AM_CONDITIONAL([USE_EGL], [test "$enable_egl" = "yes"])
AM_CONDITIONAL([USE_GLES2], [test "$enable_gles2" = "yes"])
AM_CONDITIONAL([USE_GLX], [test "$enable_glx" = "yes"])

# WebKit feature conditionals.
AM_CONDITIONAL([ENABLE_DEBUG],[test "$enable_debug" = "yes"])
AM_CONDITIONAL([ENABLE_WEBGL],[test "$enable_webgl" = "yes"])
AM_CONDITIONAL([ENABLE_VIDEO],[test "$enable_video" = "yes"])
AM_CONDITIONAL([ENABLE_SVG],[test "$enable_svg" = "yes"])
AM_CONDITIONAL([ENABLE_COVERAGE],[test "$enable_coverage" = "yes"])
AM_CONDITIONAL([ENABLE_WEB_AUDIO],[test "$enable_web_audio" = "yes"])
AM_CONDITIONAL([ENABLE_WEBKIT1],[test "$enable_webkit1" = "yes"])
AM_CONDITIONAL([ENABLE_WEBKIT2],[test "$enable_webkit2" = "yes"])
AM_CONDITIONAL([ENABLE_SPELLCHECK],[test "$enable_spellcheck" = "yes"])

# Introspection conditional.
AM_CONDITIONAL([ENABLE_INTROSPECTION],[test "$enable_introspection" = "yes"])

