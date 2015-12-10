@echo off
SETLOCAL EnableExtensions EnableDelayedExpansion

:: Build Qt5
pushd qtbase

set QT_CONFIG=

set QT_CONFIG=!QT_CONFIG! -%BUILD_TYPE%
set QT_CONFIG=!QT_CONFIG! -static
set QT_CONFIG=!QT_CONFIG! -opensource
set QT_CONFIG=!QT_CONFIG! -confirm-license
set QT_CONFIG=!QT_CONFIG! -nomake tests
set QT_CONFIG=!QT_CONFIG! -nomake examples
set QT_CONFIG=!QT_CONFIG! -mp
set QT_CONFIG=!QT_CONFIG! -no-cetest
set QT_CONFIG=!QT_CONFIG! -no-angle
set QT_CONFIG=!QT_CONFIG! -no-opengl
set QT_CONFIG=!QT_CONFIG! -icu
set QT_CONFIG=!QT_CONFIG! -qt-zlib
set QT_CONFIG=!QT_CONFIG! -qt-libpng
set QT_CONFIG=!QT_CONFIG! -qt-libjpeg
set QT_CONFIG=!QT_CONFIG! -openssl-linked

call configure.bat !QT_CONFIG!

call %MAKE_TOOL% %BUILD_TYPE%

popd
