list(APPEND WTF_SOURCES
    gobject/GlibUtilities.cpp
    gobject/GOwnPtr.cpp
    gobject/GRefPtr.cpp

    gtk/MainThreadGtk.cpp
)

list(APPEND WTF_LIBRARIES
    pthread
    ${GLIB_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
)

list(APPEND WTF_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
)
