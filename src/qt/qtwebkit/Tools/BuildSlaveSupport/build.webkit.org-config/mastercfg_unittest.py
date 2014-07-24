#! /usr/bin/python

import sys
import os
import StringIO
import unittest

# Show DepricationWarnings come from buildbot - it isn't default with Python 2.7 or newer.
# See https://bugs.webkit.org/show_bug.cgi?id=90161 for details.
import warnings
warnings.simplefilter('default')

class BuildBotConfigLoader(object):
    def _add_webkitpy_to_sys_path(self):
        # When files are passed to the python interpreter on the command line (e.g. python test.py) __file__ is a relative path.
        absolute_file_path = os.path.abspath(__file__)
        webkit_org_config_dir = os.path.dirname(absolute_file_path)
        build_slave_support_dir = os.path.dirname(webkit_org_config_dir)
        webkit_tools_dir = os.path.dirname(build_slave_support_dir)
        scripts_dir = os.path.join(webkit_tools_dir, 'Scripts')
        sys.path.append(scripts_dir)

    def _create_mock_passwords_dict(self):
        config_dict = json.load(open('config.json'))
        return dict([(slave['name'], '1234') for slave in config_dict['slaves']])

    def _mock_open(self, filename):
        if filename == 'passwords.json':
            return StringIO.StringIO(json.dumps(self._create_mock_passwords_dict()))
        return __builtins__.open(filename)

    def _add_dependant_modules_to_sys_modules(self):
        from webkitpy.thirdparty.autoinstalled import buildbot
        sys.modules['buildbot'] = buildbot

    def load_config(self, master_cfg_path):
        # Before we can use webkitpy.thirdparty, we need to fix our path to include webkitpy.
        # FIXME: If we're ever run by test-webkitpy we won't need this step.
        self._add_webkitpy_to_sys_path()
        # master.cfg expects the buildbot module to be in sys.path.
        self._add_dependant_modules_to_sys_modules()

        # master.cfg expects a passwords.json file which is not checked in.  Fake it by mocking open().
        globals()['open'] = self._mock_open
        # Because the master_cfg_path may have '.' in its name, we can't just use import, we have to use execfile.
        # We pass globals() as both the globals and locals to mimic exectuting in the global scope, so
        # that globals defined in master.cfg will be global to this file too.
        execfile(master_cfg_path, globals(), globals())
        globals()['open'] = __builtins__.open  # Stop mocking open().


class MasterCfgTest(unittest.TestCase):
    def test_nrwt_leaks_parsing(self):
        run_webkit_tests = RunWebKitTests()  # pylint is confused by the way we import the module ... pylint: disable-msg=E0602
        log_text = """
12:44:24.295 77706 13981 total leaks found for a total of 197,936 bytes!
12:44:24.295 77706 1 unique leaks found!
"""
        expected_incorrect_lines = [
            '13981 total leaks found for a total of 197,936 bytes!',
            '1 unique leaks found!',
        ]
        run_webkit_tests._parseNewRunWebKitTestsOutput(log_text)
        self.assertEqual(run_webkit_tests.incorrectLayoutLines, expected_incorrect_lines)

    def test_nrwt_missing_results(self):
        run_webkit_tests = RunWebKitTests()  # pylint is confused by the way we import the module ... pylint: disable-msg=E0602
        log_text = """
Expected to fail, but passed: (2)
  animations/additive-transform-animations.html
  animations/cross-fade-webkit-mask-box-image.html

Unexpected flakiness: text-only failures (2)
  fast/events/touch/touch-inside-iframe.html [ Failure Pass ]
  http/tests/inspector-enabled/console-clear-arguments-on-frame-navigation.html [ Failure Pass ]

Unexpected flakiness: timeouts (1)
  svg/text/foreignObject-repaint.xml [ Timeout Pass ]

Regressions: Unexpected missing results (1)
  svg/custom/zero-path-square-cap-rendering2.svg [ Missing ]

Regressions: Unexpected text-only failures (1)
  svg/custom/zero-path-square-cap-rendering2.svg [ Failure ]
"""
        run_webkit_tests._parseNewRunWebKitTestsOutput(log_text)
        self.assertEqual(set(run_webkit_tests.incorrectLayoutLines),
            set(['2 new passes', '3 flakes', '1 missing results', '1 failures']))

class StubStdio(object):
    def __init__(self, stdio):
        self._stdio = stdio

    def getText(self):
        return self._stdio


class StubRemoteCommand(object):
    def __init__(self, rc, stdio):
        self.rc = rc
        self.logs = {'stdio': StubStdio(stdio)}


class RunQtAPITestsTest(unittest.TestCase):
    def assertResults(self, expected_result, expected_text, stdio):
        rc = 0
        cmd = StubRemoteCommand(rc, stdio)
        step = RunQtAPITests()
        step.commandComplete(cmd)
        actual_results = step.evaluateCommand(cmd)
        actual_text = str(step.getText2(cmd, actual_results)[0])

        self.assertEqual(expected_result, actual_results)
        self.assertEqual(actual_text, expected_text)

    def test_timeout(self):
        self.assertResults(FAILURE, "API tests", """INFO:Exec:Running... WebKitBuild/Release/Source/WebKit2/UIProcess/API/qt/tests/qquickwebview/tst_qquickwebview
INFO:Exec:Running... WebKitBuild/Release/Source/WebKit2/UIProcess/API/qt/tests/qmltests/tst_qmltests
Qml debugging is enabled. Only use this in a safe environment!
INFO:Exec:Finished WebKitBuild/Release/Source/WebKit2/UIProcess/API/qt/tests/qquickwebview/tst_qquickwebview
ERROR:Exec:Timeout, process 'WebKitBuild/Release/Source/WebKit2/UIProcess/API/qt/tests/qmltests/tst_qmltests' (2336) was terminated
INFO:Exec:Finished WebKitBuild/Release/Source/WebKit2/UIProcess/API/qt/tests/qmltests/tst_qmltests
********* Start testing of tst_QQuickWebView *********
Config: Using QTest library 5.0.0, Qt 5.0.0
PASS   : tst_QQuickWebView::initTestCase()
QWARN  : tst_QQuickWebView::accessPage() QQuickCanvas: platform does not support threaded rendering!
.
.
.

**********************************************************************
**        TOTALS: 16 passed, 0 failed, 0 skipped, 0 crashed         **
**********************************************************************""")

    def test_success(self):
        self.assertResults(SUCCESS, "API tests", """INFO:Exec:Running... WebKitBuild/Release/Source/WebKit2/UIProcess/API/qt/tests/qquickwebview/tst_qquickwebview
INFO:Exec:Running... WebKitBuild/Release/Source/WebKit2/UIProcess/API/qt/tests/qmltests/tst_qmltests
Qml debugging is enabled. Only use this in a safe environment!
INFO:Exec:Finished WebKitBuild/Release/Source/WebKit2/UIProcess/API/qt/tests/qquickwebview/tst_qquickwebview
********* Start testing of tst_QQuickWebView *********
Config: Using QTest library 5.0.0, Qt 5.0.0
PASS   : tst_QQuickWebView::initTestCase()
QWARN  : tst_QQuickWebView::accessPage() QQuickCanvas: platform does not support threaded rendering!
.
.
.

**********************************************************************
**        TOTALS: 16 passed, 0 failed, 0 skipped, 0 crashed         **
**********************************************************************""")

    def test_failure(self):
        self.assertResults(WARNINGS, "16 passed, 1 failed, 0 skipped, 0 crashed", """********* Start testing of tst_QDeclarativeWebView *********
PASS   : tst_QDeclarativeWebView::pressGrabTime()
PASS   : tst_QDeclarativeWebView::renderingEnabled()
PASS   : tst_QDeclarativeWebView::setHtml()
PASS   : tst_QDeclarativeWebView::settings()
FAIL!  : tst_QDeclarativeWebView::backgroundColor() Compared values are not the same
   Loc: [/ramdisk/qt-linux-release/build/Source/WebKit/qt/tests/qdeclarativewebview/tst_qdeclarativewebview.cpp(532)]
PASS   : tst_QDeclarativeWebView::cleanupTestCase()
.
.
.

**********************************************************************
**        TOTALS: 16 passed, 1 failed, 0 skipped, 0 crashed         **
**********************************************************************""")

    def test_timeout_and_failure(self):
        self.assertResults(FAILURE, "Failure: timeout occured during testing", """INFO:Exec:Finished WebKitBuild/Release/Source/WebKit/qt/tests/benchmarks/painting/tst_painting
ERROR:Exec:Timeout, process 'WebKitBuild/Release/Source/WebKit/qt/tests/qwebpage/tst_qwebpage' (13000) was terminated
INFO:Exec:Finished WebKitBuild/Release/Source/WebKit/qt/tests/qwebpage/tst_qwebpage
********* Start testing of tst_Loading *********
Config: Using QTest library 4.8.0, Qt 4.8.0
PASS   : tst_Loading::initTestCase()
QDEBUG : tst_Loading::load(amazon) loaded the Generic plugin
RESULT : tst_Loading::load():"amazon":
     1,843 msecs per iteration (total: 1,843, iterations: 1)
RESULT : tst_Loading::load():"kde":
     139 msecs per iteration (total: 139, iterations: 1)
RESULT : tst_Loading::load():"apple":
     740 msecs per iteration (total: 740, iterations: 1)
PASS   : tst_Loading::load()
PASS   : tst_Loading::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped
********* Finished testing of tst_Loading *********
.
.
.
PASS   : tst_QDeclarativeWebView::renderingEnabled()
PASS   : tst_QDeclarativeWebView::setHtml()
PASS   : tst_QDeclarativeWebView::settings()
FAIL!  : tst_QDeclarativeWebView::backgroundColor() Compared values are not the same
   Loc: [/ramdisk/qt-linux-release/build/Source/WebKit/qt/tests/qdeclarativewebview/tst_qdeclarativewebview.cpp(532)]
PASS   : tst_QDeclarativeWebView::cleanupTestCase()
Totals: 16 passed, 3 failed, 1 skipped
.
.
.
**********************************************************************
**        TOTALS: 73 passed, 3 failed, 1 skipped, 0 crashed         **
**********************************************************************""")

    def test_crash(self):
        self.assertResults(FAILURE, "API tests", """********* Start testing of tst_QQuickWebView *********
Config: Using QTest library 5.0.0, Qt 5.0.0
PASS   : tst_QQuickWebView::initTestCase()
PASS   : tst_QQuickWebView::accessPage()

CRASHED: WebKitBuild/Release/Source/WebKit2/UIProcess/API/qt/tests/qquickwebview/tst_qquickwebview

CRASHED: WebKitBuild/Release/Source/WebKit2/UIProcess/API/qt/tests/qquickwebview/tst_hello

********* Start testing of tst_publicapi *********
Config: Using QTest library 5.0.0, Qt 5.0.0
PASS   : tst_publicapi::initTestCase()
PASS   : tst_publicapi::publicAPI()
PASS   : tst_publicapi::cleanupTestCase()
Totals: 3 passed, 0 failed, 0 skipped
********* Finished testing of tst_publicapi *********
**********************************************************************
**        TOTALS: 92 passed, 0 failed, 0 skipped, 2 crashed         **
**********************************************************************""")



class RunUnitTestsTest(unittest.TestCase):
    def assertFailures(self, expected_failure_count, stdio):
        if expected_failure_count:
            rc = 1
            expected_results = FAILURE
            expected_text = '{0} unit tests failed or timed out'.format(expected_failure_count)
        else:
            rc = 0
            expected_results = SUCCESS
            expected_text = 'run-api-tests'

        cmd = StubRemoteCommand(rc, stdio)
        step = RunUnitTests()
        step.commandComplete(cmd)
        actual_results = step.evaluateCommand(cmd)
        actual_failure_count = step.failedTestCount
        actual_text = step.getText(cmd, actual_results)[0]

        self.assertEqual(expected_results, actual_results)
        self.assertEqual(expected_failure_count, actual_failure_count)
        self.assertEqual(expected_text, actual_text)

    def test_no_failures_or_timeouts(self):
        self.assertFailures(0, """Note: Google Test filter = WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[==========] Running 1 test from 1 test case.
[----------] Global test environment set-up.
[----------] 1 test from WebViewDestructionWithHostWindow
[ RUN      ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[       OK ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose (127 ms)
[----------] 1 test from WebViewDestructionWithHostWindow (127 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test case ran. (127 ms total)
[  PASSED  ] 1 test.
""")

    def test_one_failure(self):
        self.assertFailures(1, """Note: Google Test filter = WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[==========] Running 1 test from 1 test case.
[----------] Global test environment set-up.
[----------] 1 test from WebViewDestructionWithHostWindow
[ RUN      ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[       OK ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose (127 ms)
[----------] 1 test from WebViewDestructionWithHostWindow (127 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test case ran. (127 ms total)
[  PASSED  ] 1 test.
Tests that failed:
  WebKit2.WebKit2.CanHandleRequest
""")

    def test_multiple_failures(self):
        self.assertFailures(4, """Note: Google Test filter = WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[==========] Running 1 test from 1 test case.
[----------] Global test environment set-up.
[----------] 1 test from WebViewDestructionWithHostWindow
[ RUN      ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[       OK ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose (127 ms)
[----------] 1 test from WebViewDestructionWithHostWindow (127 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test case ran. (127 ms total)
[  PASSED  ] 1 test.
Tests that failed:
  WebKit2.WebKit2.CanHandleRequest
  WebKit2.WebKit2.DocumentStartUserScriptAlertCrashTest
  WebKit2.WebKit2.HitTestResultNodeHandle
  WebKit2.WebKit2.InjectedBundleBasic
""")

    def test_one_timeout(self):
        self.assertFailures(1, """Note: Google Test filter = WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[==========] Running 1 test from 1 test case.
[----------] Global test environment set-up.
[----------] 1 test from WebViewDestructionWithHostWindow
[ RUN      ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[       OK ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose (127 ms)
[----------] 1 test from WebViewDestructionWithHostWindow (127 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test case ran. (127 ms total)
[  PASSED  ] 1 test.
Tests that timed out:
  WebKit2.WebKit2.CanHandleRequest
""")

    def test_multiple_timeouts(self):
        self.assertFailures(4, """Note: Google Test filter = WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[==========] Running 1 test from 1 test case.
[----------] Global test environment set-up.
[----------] 1 test from WebViewDestructionWithHostWindow
[ RUN      ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[       OK ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose (127 ms)
[----------] 1 test from WebViewDestructionWithHostWindow (127 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test case ran. (127 ms total)
[  PASSED  ] 1 test.
Tests that timed out:
  WebKit2.WebKit2.CanHandleRequest
  WebKit2.WebKit2.DocumentStartUserScriptAlertCrashTest
  WebKit2.WebKit2.HitTestResultNodeHandle
  WebKit2.WebKit2.InjectedBundleBasic
""")

    def test_multiple_failures_and_timeouts(self):
        self.assertFailures(8, """Note: Google Test filter = WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[==========] Running 1 test from 1 test case.
[----------] Global test environment set-up.
[----------] 1 test from WebViewDestructionWithHostWindow
[ RUN      ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose
[       OK ] WebViewDestructionWithHostWindow.DestroyViewWindowWithoutClose (127 ms)
[----------] 1 test from WebViewDestructionWithHostWindow (127 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test case ran. (127 ms total)
[  PASSED  ] 1 test.
Tests that failed:
  WebKit2.WebKit2.CanHandleRequest
  WebKit2.WebKit2.DocumentStartUserScriptAlertCrashTest
  WebKit2.WebKit2.HitTestResultNodeHandle
Tests that timed out:
  WebKit2.WebKit2.InjectedBundleBasic
  WebKit2.WebKit2.LoadCanceledNoServerRedirectCallback
  WebKit2.WebKit2.MouseMoveAfterCrash
  WebKit2.WebKit2.ResponsivenessTimerDoesntFireEarly
  WebKit2.WebKit2.WebArchive
""")


class SVNMirrorTest(unittest.TestCase):
    def setUp(self):
        self.config = json.load(open('config.json'))

    def get_SVNMirrorFromConfig(self, builderName):
        SVNMirror = None
        for builder in self.config['builders']:
            if builder['name'] == builderName:
                SVNMirror = builder.pop('SVNMirror', 'http://svn.webkit.org/repository/webkit/')
        return SVNMirror

    def test_CheckOutSource(self):
        # SVN mirror feature isn't unittestable now with source.oldsource.SVN(==source.SVN) , only with source.svn.SVN(==SVN)
        # https://bugs.webkit.org/show_bug.cgi?id=85887
        if issubclass(CheckOutSource, source.SVN):
            return

        # Compare CheckOutSource.baseURL with SVNMirror (or with the default URL) in config.json for all builders
        for builder in c['builders']:
            for buildStepFactory, kwargs in builder['factory'].steps:
                if str(buildStepFactory).split('.')[-1] == 'CheckOutSource':
                        CheckOutSourceInstance = buildStepFactory(**kwargs)
                        self.assertEqual(CheckOutSourceInstance.baseURL, self.get_SVNMirrorFromConfig(builder['name']))


class BuildStepsConstructorTest(unittest.TestCase):
    # "Passing a BuildStep subclass to factory.addStep is deprecated. Please pass a BuildStep instance instead.  Support will be dropped in v0.8.7."
    # It checks if all builder's all buildsteps can be insantiated after migration.
    # https://bugs.webkit.org/show_bug.cgi?id=89001
    # http://buildbot.net/buildbot/docs/0.8.6p1/manual/customization.html#writing-buildstep-constructors

    @staticmethod
    def generateTests():
        for builderNumber, builder in enumerate(c['builders']):
            for stepNumber, step in enumerate(builder['factory'].steps):
                builderName = builder['name'].encode('ascii', 'ignore')
                setattr(BuildStepsConstructorTest, 'test_builder%02d_step%02d' % (builderNumber, stepNumber), BuildStepsConstructorTest.createTest(builderName, step))

    @staticmethod
    def createTest(builderName, step):
        def doTest(self):
            try:
                buildStepFactory, kwargs = step
                buildStepFactory(**kwargs)
            except TypeError as e:
                buildStepName = str(buildStepFactory).split('.')[-1]
                self.fail("Error during instantiation %s buildstep for %s builder: %s\n" % (buildStepName, builderName, e))
        return doTest

# FIXME: We should run this file as part of test-webkitpy.
# Unfortunately test-webkitpy currently requires that unittests
# be located in a directory with a valid module name.
# 'build.webkit.org-config' is not a valid module name (due to '.' and '-')
# so for now this is a stand-alone test harness.
if __name__ == '__main__':
    BuildBotConfigLoader().load_config('master.cfg')
    BuildStepsConstructorTest.generateTests()
    unittest.main()
