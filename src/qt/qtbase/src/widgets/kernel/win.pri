# Qt/Windows only configuration file
# --------------------------------------------------------------------

INCLUDEPATH += ../3rdparty/wintab
!wince*:!winrt {
    LIBS_PRIVATE *= -lshell32
}
