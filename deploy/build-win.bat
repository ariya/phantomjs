
set ROOT=C:\Users\leo\kdab\phantomjs\deploy
set QT_VERSION=4.8
set QT_FOLDER=Qt-%QT_VERSION%
set GIT="C:\Program Files (x86)\Git\bin\git.exe"

set QT_URL=git://gitorious.org/~lfranchi/qt/lfranchi-qt.git
set COMPILE_JOBS=10

set OPENSSL_FOLDER=openssl-1.0.0e

echo "Compiling openssl. Please make sure you've downloaded and extracted openssl-1.0.0e.tar.gz..."
if not exist %OPENSSL_FOLDER% exit

cd %OPENSSL_FOLDER%
perl Configure VC-WIN32 no-asm --prefix=%ROOT%\openssl
ms\do_ms

nmake -f ms\ntdll.mak
nmake -f ms\ntdll.mak install
cd ..

set INCLUDE=%ROOT%\openssl\include;%INCLUDE%
set PATH=%ROOT%\openssl\bin;%PATH%

echo "Cloning Qt $QT_VERSION from Gitorious. Please wait..."

if exist %QT_FOLDER% goto update
%GIT% clone %QT_URL% %QT_FOLDER%
cd %QT_FOLDER%
%GIT% checkout -b phantomjs origin/phantomjs
cd ..

:update   
cd %QT_FOLDER%
%GIT% clean -xdf
%GIT% checkout -f
%GIT% pull
%GIT% checkout phantomjs


echo "Building Qt %QT_VERSION%. Please wait..."
configure.exe -opensource -confirm-license -release -no-exceptions -no-stl -no-xmlpatterns -no-phonon -no-qt3support -no-opengl -no-declarative -rtti -mmx -3dnow -sse -sse2 -openssl -no-dbus -qt-libpng -qt-libjpeg -D QT_NO_STYLE_CDE -D QT_NO_STYLE_CLEANLOOKS -D QT_NO_STYLE_MOTIF -D QT_NO_STYLE_PLASTIQUE -platform win32-msvc2010 -nomake demos -nomake examples -nomake tools
nmake
cd ..

echo "Building PhantomJS. Please wait..."
echo ""
cd ..

if exist Makefile nmake distclean

deploy\%QT_FOLDER%\bin\qmake.exe
nmake

copy deploy\%QT_FOLDER%\bin\QtCore4.dll bin\
copy deploy\%QT_FOLDER%\bin\QtGui4.dll bin\
copy deploy\%QT_FOLDER%\bin\QtNetwork4.dll bin\
copy deploy\%QT_FOLDER%\bin\QtWebKit4.dll bin\
copy deploy\openssl\bin\libeay32.dll bin\
copy deploy\openssl\bin\ssleay32.dll bin\