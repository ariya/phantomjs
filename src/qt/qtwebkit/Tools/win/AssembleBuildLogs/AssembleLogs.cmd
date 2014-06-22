if exist "%CONFIGURATIONBUILDDIR%\BuildOutput.htm" del "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WTFGenerated...                                 >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WTFGenerated\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WTF...                                          >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WTF\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING JavaScriptCoreGenerated...                      >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\JavaScriptCoreGenerated\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING LLIntDesiredOffsets...                          >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\LLIntDesiredOffsets\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING LLIntOffsetsExtractor...                        >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\LLIntOffsetsExtractor\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING LLIntAssembly...                                >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\LLIntAssembly\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING JavaScriptCore...                               >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\JavaScriptCore\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING jsc...                                          >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\jsc\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING testRegExp...                                   >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\testRegExp\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING testapi...                                      >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\testapi\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

if not exist "%CONFIGURATIONBUILDDIR%\obj32\WebKitQuartzCoreAdditions\BuildLog.htm" GOTO SkipInternalProjects

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WebKitSystemInterfaceGenerated...               >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WebKitSystemInterfaceGenerated\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WebKitSystemInterface...                        >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WebKitSystemInterface\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WebKitQuartzCoreAdditionsGenerated...           >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WebKitQuartzCoreAdditionsGenerated\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WebKitQuartzCoreAdditions...                    >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WebKitQuartzCoreAdditions\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING CoreUI...                                       >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\CoreUI\BuildLog.htm"       >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING SafariTheme...                                  >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\SafariTheme\BuildLog.htm"  >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

:SkipInternalProjects

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WebCoreGenerated...                             >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WebCoreGenerated\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING QTMovieWin...                                   >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\QTMovieWin\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WebCore...                                      >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WebCore\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WebCoreTestSupport...                           >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WebCoreTestSupport\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING Interfaces...                                   >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\Interfaces\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WebKitGUID...                                   >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WebKitGUID\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WebKitExportGenerator...                        >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WebKitExportGenerator\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WebKit...                                       >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WebKit\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WinLauncherLib...                               >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WinLauncherLib\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING WinLauncher...                                  >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\WinLauncher\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING TestNetscapePlugin...                           >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\TestNetscapePlugin\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING ImageDiff...                                    >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\ImageDiff\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING ImageDiffLauncher...                            >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\ImageDiffLauncher\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING DumpRenderTree...                               >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\DumpRenderTree\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING DumpRenderTreeLauncher...                       >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\DumpRenderTreeLauncher\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING record-memory...                                >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\record-memory\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING gtest-md...                                     >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\gtest-md\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"

echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo COMPILING TestWebKitAPI...                                >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
echo _________________________________________________________ >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"
type "%CONFIGURATIONBUILDDIR%\obj32\TestWebKitAPI\BuildLog.htm" >> "%CONFIGURATIONBUILDDIR%\BuildOutput.htm"