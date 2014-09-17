@echo off

set QT_CFG=

set QT_CFG=%QT_CFG% -opensource
set QT_CFG=%QT_CFG% -confirm-license

set QT_CFG=%QT_CFG% -release
set QT_CFG=%QT_CFG% -static

set QT_CFG=%QT_CFG% -no-exceptions
set QT_CFG=%QT_CFG% -no-stl
set QT_CFG=%QT_CFG% -no-xmlpatterns
set QT_CFG=%QT_CFG% -no-phonon
set QT_CFG=%QT_CFG% -no-qt3support
set QT_CFG=%QT_CFG% -no-opengl
set QT_CFG=%QT_CFG% -no-declarative
set QT_CFG=%QT_CFG% -no-script
set QT_CFG=%QT_CFG% -no-multimedia

set QT_CFG=%QT_CFG% -qt-zlib
set QT_CFG=%QT_CFG% -qt-libpng
set QT_CFG=%QT_CFG% -qt-libjpeg
set QT_CFG=%QT_CFG% -no-libmng
set QT_CFG=%QT_CFG% -no-libtiff

set QT_CFG=%QT_CFG% -D QT_NO_GRAPHICSVIEW
set QT_CFG=%QT_CFG% -D QT_NO_GRAPHICSEFFECT
rem set QT_CFG=%QT_CFG% -D QT_NO_NATIVE_GESTURES

set QT_CFG=%QT_CFG% -D QT_NO_STYLE_CDE
set QT_CFG=%QT_CFG% -D QT_NO_STYLE_CLEANLOOKS
set QT_CFG=%QT_CFG% -D QT_NO_STYLE_MOTIF
set QT_CFG=%QT_CFG% -D QT_NO_STYLE_PLASTIQUE

set QT_CFG=%QT_CFG% -nomake demos
set QT_CFG=%QT_CFG% -nomake examples
set QT_CFG=%QT_CFG% -nomake tools
set QT_CFG=%QT_CFG% -nomake translations

set OPEN_SSL_DIR=d:\ownprojects\OpenSSL

set QT_CFG=%QT_CFG% -openssl-linked
set QT_CFG=%QT_CFG% -I %OPEN_SSL_DIR%\include
set QT_CFG=%QT_CFG% -L %OPEN_SSL_DIR%\lib
set QT_CFG=%QT_CFG% -l libeay32
set QT_CFG=%QT_CFG% -l ssleay32
set QT_CFG=%QT_CFG% OPENSSL_LIBS="-L%OPEN_SSL_DIR:\=\\%\\lib -lssleay32 -llibeay32 -luser32 -lgdi32"

set MAKE_COMMAND=nmake
rem set QT_CFG=%QT_CFG% -make %MAKE_COMMAND%

configure %QT_CFG%

%MAKE_COMMAND%

cd src\plugins\codecs\
..\..\..\bin\qmake -r
%MAKE_COMMAND%
cd ..\..\..\

cd src\plugins\imageformats\
..\..\..\bin\qmake -r
%MAKE_COMMAND%
cd ..\..\..\

cd src\3rdparty\webkit\Source\
..\..\..\..\bin\qmake -r
%MAKE_COMMAND%
cd ..\..\..\..\
