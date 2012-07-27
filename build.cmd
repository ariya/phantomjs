@echo off

set PHANTOMJSDIR=%~dp0
set PHANTOMJSDEPLOYDIR=%PHANTOMJSDIR%\phantomjs-1.7.0-win32-static

cd %PHANTOMJSDIR%

@rem Build QtWebkit
cd src\qt
call preconfig.cmd

@rem Build PhantomJS
cd ..\..\
src\qt\bin\qmake.exe -r
nmake

@rem Create deployment directoy and copy files into it
if exist %PHANTOMJSDEPLOYDIR% rd /s /q %PHANTOMJSDEPLOYDIR%
md %PHANTOMJSDEPLOYDIR%
xcopy %PHANTOMJSDIR%\bin\phantomjs.exe %PHANTOMJSDEPLOYDIR%
xcopy %PHANTOMJSDIR%\examples %PHANTOMJSDEPLOYDIR%\examples\
xcopy %PHANTOMJSDIR%\ChangeLog %PHANTOMJSDEPLOYDIR%
xcopy %PHANTOMJSDIR%\LICENSE.BSD %PHANTOMJSDEPLOYDIR%
xcopy %PHANTOMJSDIR%\README.md %PHANTOMJSDEPLOYDIR%