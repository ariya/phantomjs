# OpenSSL support; compile in QSslSocket.
contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked) {


symbian {
	INCLUDEPATH *= $$OS_LAYER_SSL_SYSTEMINCLUDE
} else {
	include($$QT_SOURCE_TREE/config.tests/unix/openssl/openssl.pri)
}

    HEADERS += ssl/qssl.h \
               ssl/qsslcertificate.h \
               ssl/qsslcertificate_p.h \
	       ssl/qsslconfiguration.h \
	       ssl/qsslconfiguration_p.h \
               ssl/qsslcipher.h \
               ssl/qsslcipher_p.h \
               ssl/qsslerror.h \
               ssl/qsslkey.h \
               ssl/qsslsocket.h \
               ssl/qsslsocket_openssl_p.h \
               ssl/qsslsocket_openssl_symbols_p.h \
               ssl/qsslsocket_p.h
    SOURCES += ssl/qssl.cpp \
               ssl/qsslcertificate.cpp \
	       ssl/qsslconfiguration.cpp \
               ssl/qsslcipher.cpp \
               ssl/qsslerror.cpp \
               ssl/qsslkey.cpp \
               ssl/qsslsocket.cpp \
               ssl/qsslsocket_openssl.cpp \
               ssl/qsslsocket_openssl_symbols.cpp

    # Add optional SSL libs
    # Static linking of OpenSSL with msvc:
    #   - Binaries http://slproweb.com/products/Win32OpenSSL.html
    #   - also needs -lUser32 -lAdvapi32 -lGdi32 -lCrypt32
    #   - libs in <OPENSSL_DIR>\lib\VC\static
    #   - configure: -openssl -openssl-linked -I <OPENSSL_DIR>\include -L <OPENSSL_DIR>\lib\VC\static OPENSSL_LIBS="-lUser32 -lAdvapi32 -lGdi32 -lCrypt32" OPENSSL_LIBS_DEBUG="-lssleay32MDd -llibeay32MDd" OPENSSL_LIBS_RELEASE="-lssleay32MD -llibeay32MD"

    CONFIG(debug, debug|release) {
        LIBS_PRIVATE += $$OPENSSL_LIBS_DEBUG
    } else {
        LIBS_PRIVATE += $$OPENSSL_LIBS_RELEASE
    }

    LIBS_PRIVATE += $$OPENSSL_LIBS

}
