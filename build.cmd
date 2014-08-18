@echo off
SETLOCAL EnableExtensions EnableDelayedExpansion

set BUILD_TYPE=release
if /i "%1" == "debug" (
    SET BUILD_TYPE=debug
)
set ROOT_DIR=%CD%
set 3RD_PARTY_LIBRARIES_REPO_URL=https://github.com/Vitallium/phantomjs-3rdparty-win
set 3RD_PARTY_LIBRARIES_REPO_BRANCH=msvc2013

if /i BUILD_TYPE == "debug" (
    set 3RD_PARTY_LIBRARIES_REPO_BRANCH=msvc2013_debug
)

set BUILD_DATESTAMP=%date:~-4,4%%date:~-7,2%%date:~-10,2%
set BUILD_TIMESTAMP=%time:~-11,2%%time:~-8,2%

:: replace leading space with 0
set BUILD_TIMESTAMP=%BUILD_TIMESTAMP: =0%
set QT_LOG_FILE=!ROOT_DIR!\build_qt_!BUILD_DATESTAMP!-!BUILD_TIMESTAMP!.log
set WEBKIT_LOG_FILE=!ROOT_DIR!\build_webkit_!BUILD_DATESTAMP!_!BUILD_TIMESTAMP!.log
set PHANTOMJS_LOG_FILE=!ROOT_DIR!\build_phantomjs_!BUILD_DATESTAMP!_!BUILD_TIMESTAMP!.log
set MAKE_TOOL=nmake

echo:
echo Build type: !BUILD_TYPE!
call :build
ENDLOCAL

@exit /B 0

rem ========================================================================================================
:build
SETLOCAL EnableExtensions EnableDelayedExpansion
set _3RDPARTY=!ROOT_DIR!\src\qt\3rdparty

for %%X in (git.exe) do (set GIT_FOUND=%%~$PATH:X)
if defined GIT_FOUND (
    echo.
    echo GIT found. Getting 3rd party libraries.
    if not exist !_3RDPARTY! call git clone -b !3RD_PARTY_LIBRARIES_REPO_BRANCH! !3RD_PARTY_LIBRARIES_REPO_URL! !_3RDPARTY!
) else (
    ECHO.
    CALL :exitB "Git is missing! Can't proceed. Please install Git."
    GOTO :eof
)

:: prepare 3rdparty libraries
:: setup INCLUDE and LIB environment variables
:: OpenSSL
set OPENSSL_DIR=!_3RDPARTY!\openssl
set OPENSSL_LIB=!OPENSSL_DIR!\lib
set OPENSSL_INCLUDE=!OPENSSL_DIR!\include

:: ICU
set ICU_DIR=!_3RDPARTY!\libicu
set ICU_LIB=!ICU_DIR!\lib
set ICU_INCLUDE=!ICU_DIR!\include

:: libxml
set LIBXML_DIR=!_3RDPARTY!\libxml
set LIBXML_LIB=!LIBXML_DIR!\lib
set LIBXML_INCLUDE=!LIBXML_DIR!\include\libxml2

:: sqlite
set SQLITE3SRCDIR=!ROOT_DIR!\src\qt\qtbase\src\3rdparty\sqlite

set LIB=!OPENSSL_LIB!;!ICU_LIB!;!LIBXML_LIB!;%LIB%
set INCLUDE=!OPENSSL_INCLUDE!;!ICU_INCLUDE!;!LIBXML_INCLUDE!;%INCLUDE%
set PATH=!_3RDPARTY!\gnuwin32\bin;%PATH%

echo LIB: %LIB%
echo INCLUDE: %INCLUDE%

for %%X in (jom.exe) do (set JOMFOUND=%%~$PATH:X)
if defined JOMFOUND (
    set MAKE_TOOL=jom
) else (
    set MAKE_TOOL=nmake
)

pushd !ROOT_DIR!\src\qt
call preconfig.cmd !BUILD_TYPE! 2>&1 >> !QT_LOG_FILE!
popd

set PATH=!ROOT_DIR!\src\qt\qtbase\bin;%PATH%
for %%X in (qmake.exe) do (set QMAKE_FOUND=%%~$PATH:X)
if defined QMAKE_FOUND (
    echo.
    echo qmake found. Building QtWebkit
) else (
    ECHO.
    CALL :exitB "qmake.exe is missing! Can't proceed."
    GOTO :eof
)

pushd !ROOT_DIR!\src\qt\qtwebkit
call qmake.exe
%MAKE_TOOL% %BUILD_TYPE% 2>&1 >> !WEBKIT_LOG_FILE!
popd

pushd !ROOT_DIR!\src
call qmake.exe
%MAKE_TOOL% %BUILD_TYPE% 2>&1 >> !PHANTOMJS_LOG_FILE!
popd
if EXIST !ROOT_DIR!\bin\phantomjs.exe (
    echo.
    echo Build has finished
    echo.
) else (
    echo:
    echo Unable to find phantomjs.exe. Please, check log files:
    echo Qt: !QT_LOG_FILE!
    echo Webkit: !WEBKIT_LOG_FILE!
    echo PhantomJS: !PHANTOMJS_LOG_FILE!
    echo:
)
EXIT /b

rem ========================================================================================================
:: %1 an error message
:exitB
echo:
echo Error: %1
echo:
echo Contact vitaliy.slobodin@gmail.com
@exit /B 0
