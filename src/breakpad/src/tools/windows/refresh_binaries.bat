REM This batch file is meant to facilitate regenerating prebuilt binaries for 
REM the Windows tools.
REM You MUST run it from a Visual Studio xxxx Command Prompt.  To do this,
REM navigate to:
REM 
REM    Start->Programs->Microsoft Visual Studio XXXX->Tools->
REM                        Visual Studio Command Prompt
REM
REM Then run this batch file.  It performs an SVN update, edits the
REM README.binaries file to contain
REM the revision number, and builds the tools.  You must run 'svn commit' to
REM commit the pending edits to the repository.

pushd %~dp0\..\..\
call svn update --accept postpone
cd tools\windows
devenv symupload\symupload.vcproj /rebuild Release
copy symupload\Release\symupload.exe binaries\
REM switch back to top level so that 'svn info' displays useful information.
cd ..\..\
echo This checkin of the binaries was created by refresh_binaries.bat. > %TEMP%\checkin.txt
echo Date: %DATE% %TIME% >> %TEMP%\checkin.txt
echo Repository information (output of 'svn info') follows: >> %TEMP%\checkin.txt
call svn info >> %TEMP%\checkin.txt
echo Done!
echo type 'svn commit -F %%TEMP%%\checkin.txt' to commit.
popd
