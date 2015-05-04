/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/qtestcase.h>
#include <QtTest/qtestassert.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qvector.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qprocess.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlibraryinfo.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qtesttable_p.h>
#include <QtTest/qtestdata.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qsignaldumper_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/cycle_p.h>
#include <QtTest/private/qtestblacklist_p.h>

#include <numeric>
#include <algorithm>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef Q_OS_WIN
#ifndef Q_OS_WINCE
# if !defined(Q_CC_MINGW) || (defined(Q_CC_MINGW) && defined(__MINGW64_VERSION_MAJOR))
#  include <crtdbg.h>
# endif
#endif
#include <windows.h> // for Sleep
#endif
#ifdef Q_OS_UNIX
#include <errno.h>
#include <signal.h>
#include <time.h>
#endif

#if defined(Q_OS_MACX)
#include <IOKit/pwr_mgt/IOPMLib.h>
#endif

QT_BEGIN_NAMESPACE

/*!
   \namespace QTest
   \inmodule QtTest

   \brief The QTest namespace contains all the functions and
   declarations that are related to Qt Test.

   See the \l{Qt Test Overview} for information about how to write unit tests.
*/

/*! \macro QVERIFY(condition)

   \relates QTest

   The QVERIFY() macro checks whether the \a condition is true or not. If it is
   true, execution continues. If not, a failure is recorded in the test log
   and the test won't be executed further.

   \b {Note:} This macro can only be used in a test function that is invoked
   by the test framework.

   Example:
   \snippet code/src_qtestlib_qtestcase.cpp 0

   \sa QCOMPARE(), QTRY_VERIFY()
*/

/*! \macro QVERIFY2(condition, message)

    \relates QTest

    The QVERIFY2() macro behaves exactly like QVERIFY(), except that it outputs
    a verbose \a message when \a condition is false. The \a message is a plain
    C string.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 1

    \sa QVERIFY(), QCOMPARE()
*/

/*! \macro QCOMPARE(actual, expected)

   \relates QTest

   The QCOMPARE macro compares an \a actual value to an \a expected value using
   the equals operator. If \a actual and \a expected are identical, execution
   continues. If not, a failure is recorded in the test log and the test
   won't be executed further.

   In the case of comparing floats and doubles, qFuzzyCompare() is used for
   comparing. This means that comparing to 0 will likely fail. One solution
   to this is to compare to 1, and add 1 to the produced output.

   QCOMPARE tries to output the contents of the values if the comparison fails,
   so it is visible from the test log why the comparison failed.

   QCOMPARE is very strict on the data types. Both \a actual and \a expected
   have to be of the same type, otherwise the test won't compile. This prohibits
   unspecified behavior from being introduced; that is behavior that usually
   occurs when the compiler implicitly casts the argument.

   For your own classes, you can use \l QTest::toString() to format values for
   outputting into the test log.

   \note This macro can only be used in a test function that is invoked
   by the test framework.

   Example:
   \snippet code/src_qtestlib_qtestcase.cpp 2

   \sa QVERIFY(), QTRY_COMPARE(), QTest::toString()
*/

/*! \macro QVERIFY_EXCEPTION_THROWN(expression, exceptiontype)
   \since 5.3

   \relates QTest

   The QVERIFY_EXCEPTION_THROWN macro executes an \a expression and tries
   to catch an exception thrown from the \a expression. If the \a expression
   throws an exception and its type is the same as \a exceptiontype
   or \a exceptiontype is substitutable with the type of thrown exception
   (i.e. usually the type of thrown exception is publically derived
   from \a exceptiontype) then execution will be continued. If not-substitutable
   type of exception is thrown or the \a expression doesn't throw an exception
   at all, then a failure will be recorded in the test log and
   the test won't be executed further.

   \note This macro can only be used in a test function that is invoked
   by the test framework.
*/

/*! \macro QTRY_VERIFY_WITH_TIMEOUT(condition, timeout)
   \since 5.0

   \relates QTest

   The QTRY_VERIFY_WITH_TIMEOUT() macro is similar to QVERIFY(), but checks the \a condition
   repeatedly, until either the condition becomes true or the \a timeout is
   reached.  Between each evaluation, events will be processed.  If the timeout
   is reached, a failure is recorded in the test log and the test won't be
   executed further.

   \note This macro can only be used in a test function that is invoked
   by the test framework.

   \sa QTRY_VERIFY(), QVERIFY(), QCOMPARE(), QTRY_COMPARE()
*/


/*! \macro QTRY_VERIFY(condition)
   \since 5.0

   \relates QTest

   Checks the \a condition by invoking QTRY_VERIFY_WITH_TIMEOUT() with a timeout of five seconds.

   \note This macro can only be used in a test function that is invoked
   by the test framework.

   \sa QTRY_VERIFY_WITH_TIMEOUT(), QVERIFY(), QCOMPARE(), QTRY_COMPARE()
*/

/*! \macro QTRY_COMPARE_WITH_TIMEOUT(actual, expected, timeout)
   \since 5.0

   \relates QTest

   The QTRY_COMPARE_WITH_TIMEOUT() macro is similar to QCOMPARE(), but performs the comparison
   of the \a actual and \a expected values repeatedly, until either the two values
   are equal or the \a timeout is reached.  Between each comparison, events
   will be processed.  If the timeout is reached, a failure is recorded in the
   test log and the test won't be executed further.

   \note This macro can only be used in a test function that is invoked
   by the test framework.

   \sa QTRY_COMPARE(), QCOMPARE(), QVERIFY(), QTRY_VERIFY()
*/

/*! \macro QTRY_COMPARE(actual, expected)
   \since 5.0

   \relates QTest

   Performs a comparison of the \a actual and \a expected values by
   invoking QTRY_COMPARE_WITH_TIMEOUT() with a timeout of five seconds.

   \note This macro can only be used in a test function that is invoked
   by the test framework.

   \sa QTRY_COMPARE_WITH_TIMEOUT(), QCOMPARE(), QVERIFY(), QTRY_VERIFY()
*/

/*! \macro QFETCH(type, name)

   \relates QTest

   The fetch macro creates a local variable named \a name with the type \a type
   on the stack. \a name has to match the element name from the test's data.
   If no such element exists, the test will assert.

   Assuming a test has the following data:

   \snippet code/src_qtestlib_qtestcase.cpp 3

   The test data has two elements, a QString called \c aString and an integer
   called \c expected. To fetch these values in the actual test:

   \snippet code/src_qtestlib_qtestcase.cpp 4

   \c aString and \c expected are variables on the stack that are initialized with
   the current test data.

   \b {Note:} This macro can only be used in a test function that is invoked
   by the test framework. The test function must have a _data function.
*/

/*! \macro QWARN(message)

   \relates QTest
   \threadsafe

   Appends \a message as a warning to the test log. This macro can be used anywhere
   in your tests.
*/

/*! \macro QFAIL(message)

   \relates QTest

   This macro can be used to force a test failure. The test stops
   executing and the failure \a message is appended to the test log.

   \b {Note:} This macro can only be used in a test function that is invoked
   by the test framework.

   Example:

   \snippet code/src_qtestlib_qtestcase.cpp 5
*/

/*! \macro QTEST(actual, testElement)

   \relates QTest

   QTEST() is a convenience macro for \l QCOMPARE() that compares
   the value \a actual with the element \a testElement from the test's data.
   If there is no such element, the test asserts.

   Apart from that, QTEST() behaves exactly as \l QCOMPARE().

   Instead of writing:

   \snippet code/src_qtestlib_qtestcase.cpp 6

   you can write:

   \snippet code/src_qtestlib_qtestcase.cpp 7

   \sa QCOMPARE()
*/

/*! \macro QSKIP(description)

   \relates QTest

   If called from a test function, the QSKIP() macro stops execution of the test
   without adding a failure to the test log. You can use it to skip tests that
   wouldn't make sense in the current configuration. The text \a description is
   appended to the test log and should contain an explanation of why the test
   couldn't be executed.

   If the test is data-driven, each call to QSKIP() will skip only the current
   row of test data, so an unconditional call to QSKIP will produce one skip
   message in the test log for each row of test data.

   If called from an _data function, the QSKIP() macro will stop execution of
   the _data function and will prevent execution of the associated test
   function.

   If called from initTestCase() or initTestCase_data(), the QSKIP() macro will
   skip all test and _data functions.

   \b {Note:} This macro can only be used in a test function or _data
   function that is invoked by the test framework.

   Example:
   \snippet code/src_qtestlib_qtestcase.cpp 8
*/

/*! \macro QEXPECT_FAIL(dataIndex, comment, mode)

   \relates QTest

   The QEXPECT_FAIL() macro marks the next \l QCOMPARE() or \l QVERIFY() as an
   expected failure. Instead of adding a failure to the test log, an expected
   failure will be reported.

   If a \l QVERIFY() or \l QCOMPARE() is marked as an expected failure,
   but passes instead, an unexpected pass (XPASS) is written to the test log.

   The parameter \a dataIndex describes for which entry in the test data the
   failure is expected. Pass an empty string (\c{""}) if the failure
   is expected for all entries or if no test data exists.

   \a comment will be appended to the test log for the expected failure.

   \a mode is a \l QTest::TestFailMode and sets whether the test should
   continue to execute or not.

   \b {Note:} This macro can only be used in a test function that is invoked
   by the test framework.

   Example 1:
   \snippet code/src_qtestlib_qtestcase.cpp 9

   In the example above, an expected fail will be written into the test output
   if the variable \c i is not 42. If the variable \c i is 42, an unexpected pass
   is written instead. The QEXPECT_FAIL() has no influence on the second QCOMPARE()
   statement in the example.

   Example 2:
   \snippet code/src_qtestlib_qtestcase.cpp 10

   The above testfunction will not continue executing for the test data
   entry \c{data27}.

   \sa QTest::TestFailMode, QVERIFY(), QCOMPARE()
*/

/*! \macro QFINDTESTDATA(filename)
   \since 5.0

   \relates QTest

   Returns a QString for the testdata file referred to by \a filename, or an
   empty QString if the testdata file could not be found.

   This macro allows the test to load data from an external file without
   hardcoding an absolute filename into the test, or using relative paths
   which may be error prone.

   The returned path will be the first path from the following list which
   resolves to an existing file or directory:

   \list
   \li \a filename relative to QCoreApplication::applicationDirPath()
      (only if a QCoreApplication or QApplication object has been created).
   \li \a filename relative to the test's standard install directory
      (QLibraryInfo::TestsPath with the lowercased testcase name appended).
   \li \a filename relative to the directory containing the source file from which
      QFINDTESTDATA is invoked.
   \endlist

   If the named file/directory does not exist at any of these locations,
   a warning is printed to the test log.

   For example, in this code:
   \snippet code/src_qtestlib_qtestcase.cpp 26

   The testdata file will be resolved as the first existing file from:

   \list
   \li \c{/home/user/build/myxmlparser/tests/tst_myxmlparser/testxml/simple1.xml}
   \li \c{/usr/local/Qt-5.0.0/tests/tst_myxmlparser/testxml/simple1.xml}
   \li \c{/home/user/sources/myxmlparser/tests/tst_myxmlparser/testxml/simple1.xml}
   \endlist

   This allows the test to find its testdata regardless of whether the
   test has been installed, and regardless of whether the test's build tree
   is equal to the test's source tree.

   \b {Note:} reliable detection of testdata from the source directory requires
   either that qmake is used, or the \c{QT_TESTCASE_BUILDDIR} macro is defined to
   point to the working directory from which the compiler is invoked, or only
   absolute paths to the source files are passed to the compiler. Otherwise, the
   absolute path of the source directory cannot be determined.

   \b {Note:} For tests that use the \l QTEST_APPLESS_MAIN() macro to generate a
   \c{main()} function, \c{QFINDTESTDATA} will not attempt to find test data
   relative to QCoreApplication::applicationDirPath().  In practice, this means that
   tests using \c{QTEST_APPLESS_MAIN()} will fail to find their test data
   if run from a shadow build tree.
*/

/*! \macro QTEST_MAIN(TestClass)

    \relates QTest

    Implements a main() function that instantiates an application object and
    the \a TestClass, and executes all tests in the order they were defined.
    Use this macro to build stand-alone executables.

    If \c QT_WIDGETS_LIB is defined, the application object will be a QApplication,
    if \c QT_GUI_LIB is defined, the application object will be a QGuiApplication,
    otherwise it will be a QCoreApplication.  If qmake is used and the configuration
    includes \c{QT += widgets}, then \c QT_WIDGETS_LIB will be defined automatically.
    Similarly, if qmake is used and the configuration includes \c{QT += gui}, then
    \c QT_GUI_LIB will be defined automatically.

    \b {Note:} On platforms that have keypad navigation enabled by default,
    this macro will forcefully disable it if \c QT_WIDGETS_LIB is defined.  This is done
    to simplify the usage of key events when writing autotests. If you wish to write a
    test case that uses keypad navigation, you should enable it either in the
    \c {initTestCase()} or \c {init()} functions of your test case by calling
    \l {QApplication::setNavigationMode()}.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 11

    \sa QTEST_APPLESS_MAIN(), QTEST_GUILESS_MAIN(), QTest::qExec(),
    QApplication::setNavigationMode()
*/

/*! \macro QTEST_APPLESS_MAIN(TestClass)

    \relates QTest

    Implements a main() function that executes all tests in \a TestClass.

    Behaves like \l QTEST_MAIN(), but doesn't instantiate a QApplication
    object. Use this macro for really simple stand-alone non-GUI tests.

    \sa QTEST_MAIN()
*/

/*! \macro QTEST_GUILESS_MAIN(TestClass)
    \since 5.0

    \relates QTest

    Implements a main() function that instantiates a QCoreApplication object
    and the \a TestClass, and executes all tests in the order they were
    defined.  Use this macro to build stand-alone executables.

    Behaves like \l QTEST_MAIN(), but instantiates a QCoreApplication instead
    of the QApplication object. Use this macro if your test case doesn't need
    functionality offered by QApplication, but the event loop is still necessary.

    \sa QTEST_MAIN()
*/

/*!
    \macro QBENCHMARK

    \relates QTest

    This macro is used to measure the performance of code within a test.
    The code to be benchmarked is contained within a code block following
    this macro.

    For example:

    \snippet code/src_qtestlib_qtestcase.cpp 27

    \sa {Qt Test Overview#Creating a Benchmark}{Creating a Benchmark},
        {Chapter 5: Writing a Benchmark}{Writing a Benchmark}
*/

/*!
    \macro QBENCHMARK_ONCE
    \since 4.6

    \relates QTest

    \brief The QBENCHMARK_ONCE macro is for measuring performance of a
    code block by running it once.

    This macro is used to measure the performance of code within a test.
    The code to be benchmarked is contained within a code block following
    this macro.

    Unlike QBENCHMARK, the contents of the contained code block is only run
    once. The elapsed time will be reported as "0" if it's to short to
    be measured by the selected backend. (Use)

    \sa {Qt Test Overview#Creating a Benchmark}{Creating a Benchmark},
    {Chapter 5: Writing a Benchmark}{Writing a Benchmark}
*/

/*! \enum QTest::TestFailMode

    This enum describes the modes for handling an expected failure of the
    \l QVERIFY() or \l QCOMPARE() macros.

    \value Abort Aborts the execution of the test. Use this mode when it
           doesn't make sense to execute the test any further after the
           expected failure.

    \value Continue Continues execution of the test after the expected failure.

    \sa QEXPECT_FAIL()
*/

/*! \enum QTest::KeyAction

    This enum describes possible actions for key handling.

    \value Press    The key is pressed.
    \value Release  The key is released.
    \value Click    The key is clicked (pressed and released).
*/

/*! \enum QTest::MouseAction

    This enum describes possible actions for mouse handling.

    \value MousePress    A mouse button is pressed.
    \value MouseRelease  A mouse button is released.
    \value MouseClick    A mouse button is clicked (pressed and released).
    \value MouseDClick   A mouse button is double clicked (pressed and released twice).
    \value MouseMove     The mouse pointer has moved.
*/

/*! \fn void QTest::keyClick(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)

    Simulates clicking of \a key with an optional \a modifier on a \a widget.
    If \a delay is larger than 0, the test will wait for \a delay milliseconds
    before clicking the key.

    Examples:
    \snippet code/src_qtestlib_qtestcase.cpp 14

    The first example above simulates clicking the \c escape key on \c
    myWidget without any keyboard modifiers and without delay. The
    second example simulates clicking \c shift-escape on \c myWidget
    following a 200 ms delay of the test.

    \sa QTest::keyClicks()
*/

/*! \fn void QTest::keyClick(QWidget *widget, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload

    Simulates clicking of \a key with an optional \a modifier on a \a widget.
    If \a delay is larger than 0, the test will wait for \a delay milliseconds
    before clicking the key.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 13

    The example above simulates clicking \c a on \c myWidget without
    any keyboard modifiers and without delay of the test.

    \sa QTest::keyClicks()
*/

/*! \fn void QTest::keyClick(QWindow *window, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload
    \since 5.0

    Simulates clicking of \a key with an optional \a modifier on a \a window.
    If \a delay is larger than 0, the test will wait for \a delay milliseconds
    before clicking the key.

    Examples:
    \snippet code/src_qtestlib_qtestcase.cpp 29

    The first example above simulates clicking the \c escape key on \c
    myWindow without any keyboard modifiers and without delay. The
    second example simulates clicking \c shift-escape on \c myWindow
    following a 200 ms delay of the test.

    \sa QTest::keyClicks()
*/

/*! \fn void QTest::keyClick(QWindow *window, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload
    \since 5.0

    Simulates clicking of \a key with an optional \a modifier on a \a window.
    If \a delay is larger than 0, the test will wait for \a delay milliseconds
    before clicking the key.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 28

    The example above simulates clicking \c a on \c myWindow without
    any keyboard modifiers and without delay of the test.

    \sa QTest::keyClicks()
*/

/*! \fn void QTest::keyEvent(KeyAction action, QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)

    Sends a Qt key event to \a widget with the given \a key and an associated \a action.
    Optionally, a keyboard \a modifier can be specified, as well as a \a delay
    (in milliseconds) of the test before sending the event.
*/

/*! \fn void QTest::keyEvent(KeyAction action, QWidget *widget, char ascii, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload

    Sends a Qt key event to \a widget with the given key \a ascii and an associated \a action.
    Optionally, a keyboard \a modifier can be specified, as well as a \a delay
    (in milliseconds) of the test before sending the event.
*/

/*! \fn void QTest::keyEvent(KeyAction action, QWindow *window, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload
    \since 5.0

    Sends a Qt key event to \a window with the given \a key and an associated \a action.
    Optionally, a keyboard \a modifier can be specified, as well as a \a delay
    (in milliseconds) of the test before sending the event.
*/

/*! \fn void QTest::keyEvent(KeyAction action, QWindow *window, char ascii, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload
    \since 5.0

    Sends a Qt key event to \a window with the given key \a ascii and an associated \a action.
    Optionally, a keyboard \a modifier can be specified, as well as a \a delay
    (in milliseconds) of the test before sending the event.
*/

/*! \fn void QTest::keyPress(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)

    Simulates pressing a \a key with an optional \a modifier on a \a widget. If \a delay
    is larger than 0, the test will wait for \a delay milliseconds before pressing the key.

    \b {Note:} At some point you should release the key using \l keyRelease().

    \sa QTest::keyRelease(), QTest::keyClick()
*/

/*! \fn void QTest::keyPress(QWidget *widget, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload

    Simulates pressing a \a key with an optional \a modifier on a \a widget.
    If \a delay is larger than 0, the test will wait for \a delay milliseconds
    before pressing the key.

    \b {Note:} At some point you should release the key using \l keyRelease().

    \sa QTest::keyRelease(), QTest::keyClick()
*/

/*! \fn void QTest::keyPress(QWindow *window, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload
    \since 5.0

    Simulates pressing a \a key with an optional \a modifier on a \a window. If \a delay
    is larger than 0, the test will wait for \a delay milliseconds before pressing the key.

    \b {Note:} At some point you should release the key using \l keyRelease().

    \sa QTest::keyRelease(), QTest::keyClick()
*/

/*! \fn void QTest::keyPress(QWindow *window, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload
    \since 5.0

    Simulates pressing a \a key with an optional \a modifier on a \a window.
    If \a delay is larger than 0, the test will wait for \a delay milliseconds
    before pressing the key.

    \b {Note:} At some point you should release the key using \l keyRelease().

    \sa QTest::keyRelease(), QTest::keyClick()
*/

/*! \fn void QTest::keyRelease(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)

    Simulates releasing a \a key with an optional \a modifier on a \a widget.
    If \a delay is larger than 0, the test will wait for \a delay milliseconds
    before releasing the key.

    \sa QTest::keyPress(), QTest::keyClick()
*/

/*! \fn void QTest::keyRelease(QWidget *widget, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload

    Simulates releasing a \a key with an optional \a modifier on a \a widget.
    If \a delay is larger than 0, the test will wait for \a delay milliseconds
    before releasing the key.

    \sa QTest::keyClick()
*/

/*! \fn void QTest::keyRelease(QWindow *window, Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload
    \since 5.0

    Simulates releasing a \a key with an optional \a modifier on a \a window.
    If \a delay is larger than 0, the test will wait for \a delay milliseconds
    before releasing the key.

    \sa QTest::keyPress(), QTest::keyClick()
*/

/*! \fn void QTest::keyRelease(QWindow *window, char key, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)
    \overload
    \since 5.0

    Simulates releasing a \a key with an optional \a modifier on a \a window.
    If \a delay is larger than 0, the test will wait for \a delay milliseconds
    before releasing the key.

    \sa QTest::keyClick()
*/

/*! \fn void QTest::keyClicks(QWidget *widget, const QString &sequence, Qt::KeyboardModifiers modifier = Qt::NoModifier, int delay=-1)

    Simulates clicking a \a sequence of keys on a \a
    widget. Optionally, a keyboard \a modifier can be specified as
    well as a \a delay (in milliseconds) of the test before each key
    click.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 15

    The example above simulates clicking the sequence of keys
    representing "hello world" on \c myWidget without any keyboard
    modifiers and without delay of the test.

    \sa QTest::keyClick()
*/

/*! \fn void QTest::waitForEvents()
    \internal
*/

/*! \fn void QTest::mouseEvent(MouseAction action, QWidget *widget, Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos, int delay=-1)
    \internal
*/

/*! \fn void QTest::mouseEvent(MouseAction action, QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers stateKey, QPoint pos, int delay=-1)
    \internal
*/

/*! \fn void QTest::mousePress(QWidget *widget, Qt::MouseButton button, Qt::KeyboardModifiers modifier = 0, QPoint pos = QPoint(), int delay=-1)

    Simulates pressing a mouse \a button with an optional \a modifier
    on a \a widget.  The position is defined by \a pos; the default
    position is the center of the widget. If \a delay is specified,
    the test will wait for the specified amount of milliseconds before
    the press.

    \sa QTest::mouseRelease(), QTest::mouseClick()
*/

/*! \fn void QTest::mousePress(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0, QPoint pos = QPoint(), int delay=-1)
    \overload
    \since 5.0

    Simulates pressing a mouse \a button with an optional \a stateKey modifier
    on a \a window.  The position is defined by \a pos; the default
    position is the center of the window. If \a delay is specified,
    the test will wait for the specified amount of milliseconds before
    the press.

    \sa QTest::mouseRelease(), QTest::mouseClick()
*/

/*! \fn void QTest::mouseRelease(QWidget *widget, Qt::MouseButton button, Qt::KeyboardModifiers modifier = 0, QPoint pos = QPoint(), int delay=-1)

    Simulates releasing a mouse \a button with an optional \a modifier
    on a \a widget.  The position of the release is defined by \a pos;
    the default position is the center of the widget. If \a delay is
    specified, the test will wait for the specified amount of
    milliseconds before releasing the button.

    \sa QTest::mousePress(), QTest::mouseClick()
*/

/*! \fn void QTest::mouseRelease(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0, QPoint pos = QPoint(), int delay=-1)
    \overload
    \since 5.0

    Simulates releasing a mouse \a button with an optional \a stateKey modifier
    on a \a window.  The position of the release is defined by \a pos;
    the default position is the center of the window. If \a delay is
    specified, the test will wait for the specified amount of
    milliseconds before releasing the button.

    \sa QTest::mousePress(), QTest::mouseClick()
*/

/*! \fn void QTest::mouseClick(QWidget *widget, Qt::MouseButton button, Qt::KeyboardModifiers modifier = 0, QPoint pos = QPoint(), int delay=-1)

    Simulates clicking a mouse \a button with an optional \a modifier
    on a \a widget.  The position of the click is defined by \a pos;
    the default position is the center of the widget. If \a delay is
    specified, the test will wait for the specified amount of
    milliseconds before pressing and before releasing the button.

    \sa QTest::mousePress(), QTest::mouseRelease()
*/

/*! \fn void QTest::mouseClick(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0, QPoint pos = QPoint(), int delay=-1)
    \overload
    \since 5.0

    Simulates clicking a mouse \a button with an optional \a stateKey modifier
    on a \a window.  The position of the click is defined by \a pos;
    the default position is the center of the window. If \a delay is
    specified, the test will wait for the specified amount of
    milliseconds before pressing and before releasing the button.

    \sa QTest::mousePress(), QTest::mouseRelease()
*/

/*! \fn void QTest::mouseDClick(QWidget *widget, Qt::MouseButton button, Qt::KeyboardModifiers modifier = 0, QPoint pos = QPoint(), int delay=-1)

    Simulates double clicking a mouse \a button with an optional \a
    modifier on a \a widget.  The position of the click is defined by
    \a pos; the default position is the center of the widget. If \a
    delay is specified, the test will wait for the specified amount of
    milliseconds before each press and release.

    \sa QTest::mouseClick()
*/

/*! \fn void QTest::mouseDClick(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = 0, QPoint pos = QPoint(), int delay=-1)
    \overload
    \since 5.0

    Simulates double clicking a mouse \a button with an optional \a stateKey
    modifier on a \a window.  The position of the click is defined by
    \a pos; the default position is the center of the window. If \a
    delay is specified, the test will wait for the specified amount of
    milliseconds before each press and release.

    \sa QTest::mouseClick()
*/

/*! \fn void QTest::mouseMove(QWidget *widget, QPoint pos = QPoint(), int delay=-1)

    Moves the mouse pointer to a \a widget. If \a pos is not
    specified, the mouse pointer moves to the center of the widget. If
    a \a delay (in milliseconds) is given, the test will wait before
    moving the mouse pointer.
*/

/*! \fn void QTest::mouseMove(QWindow *window, QPoint pos = QPoint(), int delay=-1)
    \overload
    \since 5.0

    Moves the mouse pointer to a \a window. If \a pos is not
    specified, the mouse pointer moves to the center of the window. If
    a \a delay (in milliseconds) is given, the test will wait before
    moving the mouse pointer.
*/

/*!
    \fn char *QTest::toString(const T &value)

    Returns a textual representation of \a value. This function is used by
    \l QCOMPARE() to output verbose information in case of a test failure.

    You can add specializations of this function to your test to enable
    verbose output.

    \b {Note:} The caller of toString() must delete the returned data
    using \c{delete[]}.  Your implementation should return a string
    created with \c{new[]} or qstrdup().

    Example:

    \snippet code/src_qtestlib_qtestcase.cpp 16

    The example above defines a toString() specialization for a class
    called \c MyPoint. Whenever a comparison of two instances of \c
    MyPoint fails, \l QCOMPARE() will call this function to output the
    contents of \c MyPoint to the test log.

    \sa QCOMPARE()
*/

/*!
    \fn char *QTest::toString(const QLatin1String &string)
    \overload

    Returns a textual representation of the given \a string.
*/

/*!
    \fn char *QTest::toString(const QString &string)
    \overload

    Returns a textual representation of the given \a string.
*/

/*!
    \fn char *QTest::toString(const QByteArray &ba)
    \overload

    Returns a textual representation of the byte array \a ba.

    \sa QTest::toHexRepresentation()
*/

/*!
    \fn char *QTest::toString(const QTime &time)
    \overload

    Returns a textual representation of the given \a time.
*/

/*!
    \fn char *QTest::toString(const QDate &date)
    \overload

    Returns a textual representation of the given \a date.
*/

/*!
    \fn char *QTest::toString(const QDateTime &dateTime)
    \overload

    Returns a textual representation of the date and time specified by
    \a dateTime.
*/

/*!
    \fn char *QTest::toString(const QChar &character)
    \overload

    Returns a textual representation of the given \a character.
*/

/*!
    \fn char *QTest::toString(const QPoint &point)
    \overload

    Returns a textual representation of the given \a point.
*/

/*!
    \fn char *QTest::toString(const QSize &size)
    \overload

    Returns a textual representation of the given \a size.
*/

/*!
    \fn char *QTest::toString(const QRect &rectangle)
    \overload

    Returns a textual representation of the given \a rectangle.
*/

/*!
    \fn char *QTest::toString(const QUrl &url)
    \since 4.4
    \overload

    Returns a textual representation of the given \a url.
*/

/*!
    \fn char *QTest::toString(const QPointF &point)
    \overload

    Returns a textual representation of the given \a point.
*/

/*!
    \fn char *QTest::toString(const QSizeF &size)
    \overload

    Returns a textual representation of the given \a size.
*/

/*!
    \fn char *QTest::toString(const QRectF &rectangle)
    \overload

    Returns a textual representation of the given \a rectangle.
*/

/*!
    \fn char *QTest::toString(const QVariant &variant)
    \overload

    Returns a textual representation of the given \a variant.
*/

/*! \fn void QTest::qWait(int ms)

    Waits for \a ms milliseconds. While waiting, events will be processed and
    your test will stay responsive to user interface events or network communication.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 17

    The code above will wait until the network server is responding for a
    maximum of about 12.5 seconds.

    \sa QTest::qSleep(), QSignalSpy::wait()
*/

/*! \fn bool QTest::qWaitForWindowExposed(QWindow *window, int timeout)
    \since 5.0

    Waits for \a timeout milliseconds or until the \a window is exposed.
    Returns \c true if \c window is exposed within \a timeout milliseconds, otherwise returns \c false.

    This is mainly useful for asynchronous systems like X11, where a window will be mapped to screen some
    time after being asked to show itself on the screen.

    \sa QTest::qWaitForWindowActive(), QWindow::isExposed()
*/

/*! \fn bool QTest::qWaitForWindowActive(QWindow *window, int timeout)
    \since 5.0

    Waits for \a timeout milliseconds or until the \a window is active.

    Returns \c true if \c window is active within \a timeout milliseconds, otherwise returns \c false.

    \sa QTest::qWaitForWindowExposed(), QWindow::isActive()
*/

/*! \fn bool QTest::qWaitForWindowExposed(QWidget *widget, int timeout)
    \since 5.0

    Waits for \a timeout milliseconds or until the \a widget's window is exposed.
    Returns \c true if \c widget's window is exposed within \a timeout milliseconds, otherwise returns \c false.

    This is mainly useful for asynchronous systems like X11, where a window will be mapped to screen some
    time after being asked to show itself on the screen.

    \sa QTest::qWaitForWindowActive()
*/

/*! \fn bool QTest::qWaitForWindowActive(QWidget *widget, int timeout)
    \since 5.0

    Waits for \a timeout milliseconds or until the \a widget's window is active.

    Returns \c true if \c widget's window is active within \a timeout milliseconds, otherwise returns \c false.

    \sa QTest::qWaitForWindowExposed(), QWidget::isActiveWindow()
*/

/*! \fn bool QTest::qWaitForWindowShown(QWidget *widget, int timeout)
    \since 5.0
    \deprecated

    Waits for \a timeout milliseconds or until the \a widget's window is exposed.
    Returns \c true if \c widget's window is exposed within \a timeout milliseconds, otherwise returns \c false.

    This function does the same as qWaitForWindowExposed().

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 24

    \sa QTest::qWaitForWindowActive(), QTest::qWaitForWindowExposed()
*/

/*!
    \class QTest::QTouchEventSequence
    \inmodule QtTest
    \since 4.6

    \brief The QTouchEventSequence class is used to simulate a sequence of touch events.

    To simulate a sequence of touch events on a specific device for a window or widget, call
    QTest::touchEvent to create a QTouchEventSequence instance. Add touch events to
    the sequence by calling press(), move(), release() and stationary(), and let the
    instance run out of scope to commit the sequence to the event system.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 25
*/

/*!
    \fn QTest::QTouchEventSequence::~QTouchEventSequence()

    Commits this sequence of touch events, unless autoCommit was disabled, and frees allocated resources.
*/

/*!
  \fn void QTest::QTouchEventSequence::commit(bool processEvents)

   Commits this sequence of touch events to the event system. Normally there is no need to call this
   function because it is called from the destructor. However, if autoCommit is disabled, the events
   only get committed upon explicitly calling this function.

   In special cases tests may want to disable the processing of the events. This can be achieved by
   setting \a processEvents to false. This results in merely queuing the events, the event loop will
   not be forced to process them.
*/

/*!
    \fn QTouchEventSequence &QTest::QTouchEventSequence::press(int touchId, const QPoint &pt, QWindow *window)
    \since 5.0

    Adds a press event for touchpoint \a touchId at position \a pt to this sequence and returns
    a reference to this QTouchEventSequence.

    The position \a pt is interpreted as relative to \a window. If \a window is the null pointer, then
    \a pt is interpreted as relative to the window provided when instantiating this QTouchEventSequence.

    Simulates that the user pressed the touch screen or pad with the finger identified by \a touchId.
*/

/*!
    \fn QTouchEventSequence &QTest::QTouchEventSequence::press(int touchId, const QPoint &pt, QWidget *widget)

    Adds a press event for touchpoint \a touchId at position \a pt to this sequence and returns
    a reference to this QTouchEventSequence.

    The position \a pt is interpreted as relative to \a widget. If \a widget is the null pointer, then
    \a pt is interpreted as relative to the widget provided when instantiating this QTouchEventSequence.

    Simulates that the user pressed the touch screen or pad with the finger identified by \a touchId.
*/

/*!
    \fn QTouchEventSequence &QTest::QTouchEventSequence::move(int touchId, const QPoint &pt, QWindow *window)
    \since 5.0

    Adds a move event for touchpoint \a touchId at position \a pt to this sequence and returns
    a reference to this QTouchEventSequence.

    The position \a pt is interpreted as relative to \a window. If \a window is the null pointer, then
    \a pt is interpreted as relative to the window provided when instantiating this QTouchEventSequence.

    Simulates that the user moved the finger identified by \a touchId.
*/

/*!
    \fn QTouchEventSequence &QTest::QTouchEventSequence::move(int touchId, const QPoint &pt, QWidget *widget)

    Adds a move event for touchpoint \a touchId at position \a pt to this sequence and returns
    a reference to this QTouchEventSequence.

    The position \a pt is interpreted as relative to \a widget. If \a widget is the null pointer, then
    \a pt is interpreted as relative to the widget provided when instantiating this QTouchEventSequence.

    Simulates that the user moved the finger identified by \a touchId.
*/

/*!
    \fn QTouchEventSequence &QTest::QTouchEventSequence::release(int touchId, const QPoint &pt, QWindow *window)
    \since 5.0

    Adds a release event for touchpoint \a touchId at position \a pt to this sequence and returns
    a reference to this QTouchEventSequence.

    The position \a pt is interpreted as relative to \a window. If \a window is the null pointer, then
    \a pt is interpreted as relative to the window provided when instantiating this QTouchEventSequence.

    Simulates that the user lifted the finger identified by \a touchId.
*/

/*!
    \fn QTouchEventSequence &QTest::QTouchEventSequence::release(int touchId, const QPoint &pt, QWidget *widget)

    Adds a release event for touchpoint \a touchId at position \a pt to this sequence and returns
    a reference to this QTouchEventSequence.

    The position \a pt is interpreted as relative to \a widget. If \a widget is the null pointer, then
    \a pt is interpreted as relative to the widget provided when instantiating this QTouchEventSequence.

    Simulates that the user lifted the finger identified by \a touchId.
*/

/*!
    \fn QTouchEventSequence &QTest::QTouchEventSequence::stationary(int touchId)

    Adds a stationary event for touchpoint \a touchId to this sequence and returns
    a reference to this QTouchEventSequence.

    Simulates that the user did not move the finger identified by \a touchId.
*/

/*!
    \fn QTouchEventSequence QTest::touchEvent(QWindow *window, QTouchDevice *device, bool autoCommit = true)
    \since 5.0

    Creates and returns a QTouchEventSequence for the \a device to
    simulate events for \a window.

    When adding touch events to the sequence, \a window will also be used to translate
    the position provided to screen coordinates, unless another window is provided in the
    respective calls to press(), move() etc.

    The touch events are committed to the event system when the destructor of the
    QTouchEventSequence is called (ie when the object returned runs out of scope), unless
    \a autoCommit is set to false. When \a autoCommit is false, commit() has to be called
    manually.
*/

/*!
    \fn QTouchEventSequence QTest::touchEvent(QWidget *widget, QTouchDevice *device, bool autoCommit = true)

    Creates and returns a QTouchEventSequence for the \a device to
    simulate events for \a widget.

    When adding touch events to the sequence, \a widget will also be used to translate
    the position provided to screen coordinates, unless another widget is provided in the
    respective calls to press(), move() etc.

    The touch events are committed to the event system when the destructor of the
    QTouchEventSequence is called (ie when the object returned runs out of scope), unless
    \a autoCommit is set to false. When \a autoCommit is false, commit() has to be called
    manually.
*/

static bool installCoverageTool(const char * appname, const char * testname)
{
#ifdef __COVERAGESCANNER__
    if (!qEnvironmentVariableIsEmpty("QT_TESTCOCOON_ACTIVE"))
        return false;
    // Set environment variable QT_TESTCOCOON_ACTIVE to prevent an eventual subtest from
    // being considered as a stand-alone test regarding the coverage analysis.
    qputenv("QT_TESTCOCOON_ACTIVE", "1");

    // Install Coverage Tool
    __coveragescanner_install(appname);
    __coveragescanner_testname(testname);
    __coveragescanner_clear();
    return true;
#else
    Q_UNUSED(appname);
    Q_UNUSED(testname);
    return false;
#endif
}

namespace QTest
{
    static QObject *currentTestObject = 0;

    class TestFunction {
    public:
        TestFunction() : function_(-1), data_(0) {}
        void set(int function, char *data) { function_ = function; data_ = data; }
        char *data() const { return data_; }
        int function() const { return function_; }
        ~TestFunction() { delete[] data_; }
    private:
        int function_;
        char *data_;
    };
    /**
     * Contains the list of test functions that was supplied
     * on the command line, if any. Hence, if not empty,
     * those functions should be run instead of
     * all appearing in the test case.
     */
    static TestFunction * testFuncs = 0;
    static int testFuncCount = 0;

    /** Don't leak testFuncs on exit even on error */
    static struct TestFuncCleanup
    {
        void cleanup()
        {
            delete[] testFuncs;
            testFuncCount = 0;
            testFuncs = 0;
        }

        ~TestFuncCleanup() { cleanup(); }
    } testFuncCleaner;

    static int keyDelay = -1;
    static int mouseDelay = -1;
    static int eventDelay = -1;
    static bool noCrashHandler = false;

/*! \internal
    Invoke a method of the object without generating warning if the method does not exist
 */
static void invokeMethod(QObject *obj, const char *methodName)
{
    const QMetaObject *metaObject = obj->metaObject();
    int funcIndex = metaObject->indexOfMethod(methodName);
    if (funcIndex >= 0) {
        QMetaMethod method = metaObject->method(funcIndex);
        method.invoke(obj, Qt::DirectConnection);
    }
}

int defaultEventDelay()
{
    if (eventDelay == -1) {
        const QByteArray env = qgetenv("QTEST_EVENT_DELAY");
        if (!env.isEmpty())
            eventDelay = atoi(env.constData());
        else
            eventDelay = 0;
    }
    return eventDelay;
}

int Q_TESTLIB_EXPORT defaultMouseDelay()
{
    if (mouseDelay == -1) {
        const QByteArray env = qgetenv("QTEST_MOUSEEVENT_DELAY");
        if (!env.isEmpty())
            mouseDelay = atoi(env.constData());
        else
            mouseDelay = defaultEventDelay();
    }
    return mouseDelay;
}

int Q_TESTLIB_EXPORT defaultKeyDelay()
{
    if (keyDelay == -1) {
        const QByteArray env = qgetenv("QTEST_KEYEVENT_DELAY");
        if (!env.isEmpty())
            keyDelay = atoi(env.constData());
        else
            keyDelay = defaultEventDelay();
    }
    return keyDelay;
}

static bool isValidSlot(const QMetaMethod &sl)
{
    if (sl.access() != QMetaMethod::Private || sl.parameterCount() != 0
        || sl.returnType() != QMetaType::Void || sl.methodType() != QMetaMethod::Slot)
        return false;
    QByteArray name = sl.name();
    if (name.isEmpty())
        return false;
    if (name.endsWith("_data"))
        return false;
    if (name == "initTestCase" || name == "cleanupTestCase"
        || name == "cleanup" || name == "init")
        return false;
    return true;
}

Q_TESTLIB_EXPORT bool printAvailableFunctions = false;
Q_TESTLIB_EXPORT QStringList testFunctions;
Q_TESTLIB_EXPORT QStringList testTags;

static void qPrintTestSlots(FILE *stream, const char *filter = 0)
{
    for (int i = 0; i < QTest::currentTestObject->metaObject()->methodCount(); ++i) {
        QMetaMethod sl = QTest::currentTestObject->metaObject()->method(i);
        if (isValidSlot(sl)) {
            const QByteArray signature = sl.methodSignature();
            if (!filter || QString::fromLatin1(signature).contains(QLatin1String(filter), Qt::CaseInsensitive))
                fprintf(stream, "%s\n", signature.constData());
        }
    }
}

static void qPrintDataTags(FILE *stream)
{
    // Avoid invoking the actual test functions, and also avoid printing irrelevant output:
    QTestLog::setPrintAvailableTagsMode();

    // Get global data tags:
    QTestTable::globalTestTable();
    invokeMethod(QTest::currentTestObject, "initTestCase_data()");
    const QTestTable *gTable = QTestTable::globalTestTable();

    const QMetaObject *currTestMetaObj = QTest::currentTestObject->metaObject();

    // Process test functions:
    for (int i = 0; i < currTestMetaObj->methodCount(); ++i) {
        QMetaMethod tf = currTestMetaObj->method(i);

        if (isValidSlot(tf)) {

            // Retrieve local tags:
            QStringList localTags;
            QTestTable table;
            char *slot = qstrdup(tf.methodSignature().constData());
            slot[strlen(slot) - 2] = '\0';
            QByteArray member;
            member.resize(qstrlen(slot) + qstrlen("_data()") + 1);
            qsnprintf(member.data(), member.size(), "%s_data()", slot);
            invokeMethod(QTest::currentTestObject, member.constData());
            for (int j = 0; j < table.dataCount(); ++j)
                localTags << QLatin1String(table.testData(j)->dataTag());

            // Print all tag combinations:
            if (gTable->dataCount() == 0) {
                if (localTags.count() == 0) {
                    // No tags at all, so just print the test function:
                    fprintf(stream, "%s %s\n", currTestMetaObj->className(), slot);
                } else {
                    // Only local tags, so print each of them:
                    for (int k = 0; k < localTags.size(); ++k)
                        fprintf(
                            stream, "%s %s %s\n",
                            currTestMetaObj->className(), slot, localTags.at(k).toLatin1().data());
                }
            } else {
                for (int j = 0; j < gTable->dataCount(); ++j) {
                    if (localTags.count() == 0) {
                        // Only global tags, so print the current one:
                        fprintf(
                            stream, "%s %s __global__ %s\n",
                            currTestMetaObj->className(), slot, gTable->testData(j)->dataTag());
                    } else {
                        // Local and global tags, so print each of the local ones and
                        // the current global one:
                        for (int k = 0; k < localTags.size(); ++k)
                            fprintf(
                                stream, "%s %s %s __global__ %s\n", currTestMetaObj->className(), slot,
                                localTags.at(k).toLatin1().data(), gTable->testData(j)->dataTag());
                    }
                }
            }

            delete[] slot;
        }
    }
}

static int qToInt(char *str)
{
    char *pEnd;
    int l = (int)strtol(str, &pEnd, 10);
    if (*pEnd != 0) {
        fprintf(stderr, "Invalid numeric parameter: '%s'\n", str);
        exit(1);
    }
    return l;
}

Q_TESTLIB_EXPORT void qtest_qParseArgs(int argc, char *argv[], bool qml)
{
    QTestLog::LogMode logFormat = QTestLog::Plain;
    const char *logFilename = 0;

    const char *testOptions =
         " New-style logging options:\n"
         " -o filename,format  : Output results to file in the specified format\n"
         "                       Use - to output to stdout\n"
         "                       Valid formats are:\n"
         "                         txt      : Plain text\n"
         "                         csv      : CSV format (suitable for benchmarks)\n"
         "                         xunitxml : XML XUnit document\n"
         "                         xml      : XML document\n"
         "                         lightxml : A stream of XML tags\n"
         "\n"
         "     *** Multiple loggers can be specified, but at most one can log to stdout.\n"
         "\n"
         " Old-style logging options:\n"
         " -o filename         : Write the output into file\n"
         " -txt                : Output results in Plain Text\n"
         " -csv                : Output results in a CSV format (suitable for benchmarks)\n"
         " -xunitxml           : Output results as XML XUnit document\n"
         " -xml                : Output results as XML document\n"
         " -lightxml           : Output results as stream of XML tags\n"
         "\n"
         "     *** If no output file is specified, stdout is assumed.\n"
         "     *** If no output format is specified, -txt is assumed.\n"
         "\n"
         " Test log detail options:\n"
         " -silent             : Log failures and fatal errors only\n"
         " -v1                 : Log the start of each testfunction\n"
         " -v2                 : Log each QVERIFY/QCOMPARE/QTEST (implies -v1)\n"
         " -vs                 : Log every signal emission and resulting slot invocations\n"
         "\n"
         "     *** The -silent and -v1 options only affect plain text output.\n"
         "\n"
         " Testing options:\n"
         " -functions          : Returns a list of current testfunctions\n"
         " -datatags           : Returns a list of current data tags.\n"
         "                       A global data tag is preceded by ' __global__ '.\n"
         " -eventdelay ms      : Set default delay for mouse and keyboard simulation to ms milliseconds\n"
         " -keydelay ms        : Set default delay for keyboard simulation to ms milliseconds\n"
         " -mousedelay ms      : Set default delay for mouse simulation to ms milliseconds\n"
         " -maxwarnings n      : Sets the maximum amount of messages to output.\n"
         "                       0 means unlimited, default: 2000\n"
         " -nocrashhandler     : Disables the crash handler\n"
         "\n"
         " Benchmarking options:\n"
#ifdef QTESTLIB_USE_VALGRIND
         " -callgrind          : Use callgrind to time benchmarks\n"
#endif
#ifdef QTESTLIB_USE_PERF_EVENTS
         " -perf               : Use Linux perf events to time benchmarks\n"
         " -perfcounter name   : Use the counter named 'name'\n"
         " -perfcounterlist    : Lists the counters available\n"
#endif
#ifdef HAVE_TICK_COUNTER
         " -tickcounter        : Use CPU tick counters to time benchmarks\n"
#endif
         " -eventcounter       : Counts events received during benchmarks\n"
         " -minimumvalue n     : Sets the minimum acceptable measurement value\n"
         " -minimumtotal n     : Sets the minimum acceptable total for repeated executions of a test function\n"
         " -iterations  n      : Sets the number of accumulation iterations.\n"
         " -median  n          : Sets the number of median iterations.\n"
         " -vb                 : Print out verbose benchmarking information.\n";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0
            || strcmp(argv[i], "/?") == 0) {
            printf(" Usage: %s [options] [testfunction[:testdata]]...\n"
                   "    By default, all testfunctions will be run.\n\n"
                   "%s", argv[0], testOptions);

            if (qml) {
                printf ("\n"
                        " QmlTest options:\n"
                        " -import dir         : Specify an import directory.\n"
                        " -input dir/file     : Specify the root directory for test cases or a single test case file.\n"
                        " -qtquick1           : Run with QtQuick 1 rather than QtQuick 2.\n"
                        " -translation file   : Specify the translation file.\n"
                        );
            }

            printf("\n"
                   " -help               : This help\n");
            exit(0);
        } else if (strcmp(argv[i], "-functions") == 0) {
            if (qml) {
                QTest::printAvailableFunctions = true;
            } else {
                qPrintTestSlots(stdout);
                exit(0);
            }
        } else if (strcmp(argv[i], "-datatags") == 0) {
            if (!qml) {
                qPrintDataTags(stdout);
                exit(0);
            }
        } else if (strcmp(argv[i], "-txt") == 0) {
            logFormat = QTestLog::Plain;
        } else if (strcmp(argv[i], "-csv") == 0) {
            logFormat = QTestLog::CSV;
        } else if (strcmp(argv[i], "-xunitxml") == 0) {
            logFormat = QTestLog::XunitXML;
        } else if (strcmp(argv[i], "-xml") == 0) {
            logFormat = QTestLog::XML;
        } else if (strcmp(argv[i], "-lightxml") == 0) {
            logFormat = QTestLog::LightXML;
        } else if (strcmp(argv[i], "-silent") == 0) {
            QTestLog::setVerboseLevel(-1);
        } else if (strcmp(argv[i], "-v1") == 0) {
            QTestLog::setVerboseLevel(1);
        } else if (strcmp(argv[i], "-v2") == 0) {
            QTestLog::setVerboseLevel(2);
        } else if (strcmp(argv[i], "-vs") == 0) {
            QSignalDumper::startDump();
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-o needs an extra parameter specifying the filename and optional format\n");
                exit(1);
            }
            ++i;
            // Do we have the old or new style -o option?
            char *filename = new char[strlen(argv[i])+1];
            char *format = new char[strlen(argv[i])+1];
            if (sscanf(argv[i], "%[^,],%s", filename, format) == 1) {
                // Old-style
                logFilename = argv[i];
            } else {
                // New-style
                if (strcmp(format, "txt") == 0)
                    logFormat = QTestLog::Plain;
                else if (strcmp(format, "csv") == 0)
                    logFormat = QTestLog::CSV;
                else if (strcmp(format, "lightxml") == 0)
                    logFormat = QTestLog::LightXML;
                else if (strcmp(format, "xml") == 0)
                    logFormat = QTestLog::XML;
                else if (strcmp(format, "xunitxml") == 0)
                    logFormat = QTestLog::XunitXML;
                else {
                    fprintf(stderr, "output format must be one of txt, csv, lightxml, xml or xunitxml\n");
                    exit(1);
                }
                if (strcmp(filename, "-") == 0 && QTestLog::loggerUsingStdout()) {
                    fprintf(stderr, "only one logger can log to stdout\n");
                    exit(1);
                }
                QTestLog::addLogger(logFormat, filename);
            }
            delete [] filename;
            delete [] format;
        } else if (strcmp(argv[i], "-eventdelay") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-eventdelay needs an extra parameter to indicate the delay(ms)\n");
                exit(1);
            } else {
                QTest::eventDelay = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-keydelay") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-keydelay needs an extra parameter to indicate the delay(ms)\n");
                exit(1);
            } else {
                QTest::keyDelay = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-mousedelay") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-mousedelay needs an extra parameter to indicate the delay(ms)\n");
                exit(1);
            } else {
                QTest::mouseDelay = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-maxwarnings") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-maxwarnings needs an extra parameter with the amount of warnings\n");
                exit(1);
            } else {
                QTestLog::setMaxWarnings(qToInt(argv[++i]));
            }
        } else if (strcmp(argv[i], "-nocrashhandler") == 0) {
            QTest::noCrashHandler = true;
#ifdef QTESTLIB_USE_VALGRIND
        } else if (strcmp(argv[i], "-callgrind") == 0) {
            if (QBenchmarkValgrindUtils::haveValgrind())
                if (QFileInfo(QDir::currentPath()).isWritable()) {
                    QBenchmarkGlobalData::current->setMode(QBenchmarkGlobalData::CallgrindParentProcess);
                } else {
                    fprintf(stderr, "WARNING: Current directory not writable. Using the walltime measurer.\n");
                }
            else {
                fprintf(stderr, "WARNING: Valgrind not found or too old. Make sure it is installed and in your path. "
                       "Using the walltime measurer.\n");
            }
        } else if (strcmp(argv[i], "-callgrindchild") == 0) { // "private" option
            QBenchmarkGlobalData::current->setMode(QBenchmarkGlobalData::CallgrindChildProcess);
            QBenchmarkGlobalData::current->callgrindOutFileBase =
                QBenchmarkValgrindUtils::outFileBase();
#endif
#ifdef QTESTLIB_USE_PERF_EVENTS
        } else if (strcmp(argv[i], "-perf") == 0) {
            if (QBenchmarkPerfEventsMeasurer::isAvailable()) {
                // perf available
                QBenchmarkGlobalData::current->setMode(QBenchmarkGlobalData::PerfCounter);
            } else {
                fprintf(stderr, "WARNING: Linux perf events not available. Using the walltime measurer.\n");
            }
        } else if (strcmp(argv[i], "-perfcounter") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-perfcounter needs an extra parameter with the name of the counter\n");
                exit(1);
            } else {
                QBenchmarkPerfEventsMeasurer::setCounter(argv[++i]);
            }
        } else if (strcmp(argv[i], "-perfcounterlist") == 0) {
            QBenchmarkPerfEventsMeasurer::listCounters();
            exit(0);
#endif
#ifdef HAVE_TICK_COUNTER
        } else if (strcmp(argv[i], "-tickcounter") == 0) {
            QBenchmarkGlobalData::current->setMode(QBenchmarkGlobalData::TickCounter);
#endif
        } else if (strcmp(argv[i], "-eventcounter") == 0) {
            QBenchmarkGlobalData::current->setMode(QBenchmarkGlobalData::EventCounter);
        } else if (strcmp(argv[i], "-minimumvalue") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-minimumvalue needs an extra parameter to indicate the minimum time(ms)\n");
                exit(1);
            } else {
                QBenchmarkGlobalData::current->walltimeMinimum = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-minimumtotal") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-minimumtotal needs an extra parameter to indicate the minimum total measurement\n");
                exit(1);
            } else {
                QBenchmarkGlobalData::current->minimumTotal = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-iterations") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-iterations needs an extra parameter to indicate the number of iterations\n");
                exit(1);
            } else {
                QBenchmarkGlobalData::current->iterationCount = qToInt(argv[++i]);
            }
        } else if (strcmp(argv[i], "-median") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "-median needs an extra parameter to indicate the number of median iterations\n");
                exit(1);
            } else {
                QBenchmarkGlobalData::current->medianIterationCount = qToInt(argv[++i]);
            }

        } else if (strcmp(argv[i], "-vb") == 0) {
            QBenchmarkGlobalData::current->verboseOutput = true;
#ifdef Q_OS_WINRT
        } else if (strncmp(argv[i], "-ServerName:", 12) == 0 ||
                   strncmp(argv[i], "-qdevel", 7) == 0) {
            continue;
#endif
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: '%s'\n\n%s", argv[i], testOptions);
            if (qml) {
                fprintf(stderr, "\nqmltest related options:\n"
                                " -import    : Specify an import directory.\n"
                                " -input     : Specify the root directory for test cases.\n"
                                " -qtquick1  : Run with QtQuick 1 rather than QtQuick 2.\n"
                       );
            }

            fprintf(stderr, "\n"
                            " -help      : This help\n");
            exit(1);
        } else if (qml) {
            // We can't check the availability of test functions until
            // we load the QML files.  So just store the data for now.
            int colon = -1;
            int offset;
            for (offset = 0; *(argv[i]+offset); ++offset) {
                if (*(argv[i]+offset) == ':') {
                    if (*(argv[i]+offset+1) == ':') {
                        // "::" is used as a test name separator.
                        // e.g. "ClickTests::test_click:row1".
                        ++offset;
                    } else {
                        colon = offset;
                        break;
                    }
                }
            }
            if (colon == -1) {
                QTest::testFunctions += QString::fromLatin1(argv[i]);
                QTest::testTags += QString();
            } else {
                QTest::testFunctions +=
                    QString::fromLatin1(argv[i], colon);
                QTest::testTags +=
                    QString::fromLatin1(argv[i] + colon + 1);
            }
        } else {
            if (!QTest::testFuncs) {
                QTest::testFuncs = new QTest::TestFunction[512];
            }

            int colon = -1;
            char buf[512], *data=0;
            int off;
            for (off = 0; *(argv[i]+off); ++off) {
                if (*(argv[i]+off) == ':') {
                    colon = off;
                    break;
                }
            }
            if (colon != -1) {
                data = qstrdup(argv[i]+colon+1);
            }
            qsnprintf(buf, qMin(512, off + 1), "%s", argv[i]); // copy text before the ':' into buf
            qsnprintf(buf + off, qMin(512 - off, 3), "()");    // append "()"
            int idx = QTest::currentTestObject->metaObject()->indexOfMethod(buf);
            if (idx < 0 || !isValidSlot(QTest::currentTestObject->metaObject()->method(idx))) {
                fprintf(stderr, "Unknown test function: '%s'. Possible matches:\n", buf);
                buf[off] = 0;
                qPrintTestSlots(stderr, buf);
                fprintf(stderr, "\n%s -functions\nlists all available test functions.\n", argv[0]);
                exit(1);
            }
            testFuncs[testFuncCount].set(idx, data);
            testFuncCount++;
            QTEST_ASSERT(QTest::testFuncCount < 512);
        }
    }

    bool installedTestCoverage = installCoverageTool(QTestResult::currentAppName(), QTestResult::currentTestObjectName());
    QTestLog::setInstalledTestCoverage(installedTestCoverage);

    // If no loggers were created by the long version of the -o command-line
    // option, create a logger using whatever filename and format were
    // set using the old-style command-line options.
    if (QTestLog::loggerCount() == 0)
        QTestLog::addLogger(logFormat, logFilename);
}

QBenchmarkResult qMedian(const QList<QBenchmarkResult> &container)
{
    const int count = container.count();
    if (count == 0)
        return QBenchmarkResult();

    if (count == 1)
        return container.front();

    QList<QBenchmarkResult> containerCopy = container;
    std::sort(containerCopy.begin(), containerCopy.end());

    const int middle = count / 2;

    // ### handle even-sized containers here by doing an aritmetic mean of the two middle items.
    return containerCopy.at(middle);
}

struct QTestDataSetter
{
    QTestDataSetter(QTestData *data)
    {
        QTestResult::setCurrentTestData(data);
    }
    ~QTestDataSetter()
    {
        QTestResult::setCurrentTestData(0);
    }
};

namespace {

qreal addResult(qreal current, const QBenchmarkResult& r)
{
    return current + r.value;
}

}

static void qInvokeTestMethodDataEntry(char *slot)
{
    /* Benchmarking: for each median iteration*/

    bool isBenchmark = false;
    int i = (QBenchmarkGlobalData::current->measurer->needsWarmupIteration()) ? -1 : 0;

    QList<QBenchmarkResult> results;
    bool minimumTotalReached = false;
    do {
        QBenchmarkTestMethodData::current->beginDataRun();

        /* Benchmarking: for each accumulation iteration*/
        bool invokeOk;
        do {
            invokeMethod(QTest::currentTestObject, "init()");
            if (QTestResult::skipCurrentTest() || QTestResult::currentTestFailed())
                break;

            QBenchmarkTestMethodData::current->result = QBenchmarkResult();
            QBenchmarkTestMethodData::current->resultAccepted = false;

            QBenchmarkGlobalData::current->context.tag =
                QLatin1String(
                    QTestResult::currentDataTag()
                    ? QTestResult::currentDataTag() : "");

            invokeOk = QMetaObject::invokeMethod(QTest::currentTestObject, slot,
                                                 Qt::DirectConnection);
            if (!invokeOk)
                QTestResult::addFailure("Unable to execute slot", __FILE__, __LINE__);

            isBenchmark = QBenchmarkTestMethodData::current->isBenchmark();

            QTestResult::finishedCurrentTestData();

            invokeMethod(QTest::currentTestObject, "cleanup()");

            // If the test isn't a benchmark, finalize the result after cleanup() has finished.
            if (!isBenchmark)
                QTestResult::finishedCurrentTestDataCleanup();

            // If this test method has a benchmark, repeat until all measurements are
            // acceptable.
            // The QBENCHMARK macro increases the number of iterations for each run until
            // this happens.
        } while (invokeOk && isBenchmark
                 && QBenchmarkTestMethodData::current->resultsAccepted() == false
                 && !QTestResult::skipCurrentTest() && !QTestResult::currentTestFailed());

        QBenchmarkTestMethodData::current->endDataRun();
        if (!QTestResult::skipCurrentTest() && !QTestResult::currentTestFailed()) {
            if (i > -1)  // iteration -1 is the warmup iteration.
                results.append(QBenchmarkTestMethodData::current->result);

            if (isBenchmark && QBenchmarkGlobalData::current->verboseOutput) {
                if (i == -1) {
                    QTestLog::info(qPrintable(
                        QString::fromLatin1("warmup stage result      : %1")
                            .arg(QBenchmarkTestMethodData::current->result.value)), 0, 0);
                } else {
                    QTestLog::info(qPrintable(
                        QString::fromLatin1("accumulation stage result: %1")
                            .arg(QBenchmarkTestMethodData::current->result.value)), 0, 0);
                }
            }
        }

        // Verify if the minimum total measurement is reached, if it was specified:
        if (QBenchmarkGlobalData::current->minimumTotal == -1) {
            minimumTotalReached = true;
        } else {
            const qreal total = std::accumulate(results.begin(), results.end(), 0.0, addResult);
            minimumTotalReached = (total >= QBenchmarkGlobalData::current->minimumTotal);
        }
    } while (isBenchmark
             && ((++i < QBenchmarkGlobalData::current->adjustMedianIterationCount()) || !minimumTotalReached)
             && !QTestResult::skipCurrentTest() && !QTestResult::currentTestFailed());

    // If the test is a benchmark, finalize the result after all iterations have finished.
    if (isBenchmark) {
        bool testPassed = !QTestResult::skipCurrentTest() && !QTestResult::currentTestFailed();
        QTestResult::finishedCurrentTestDataCleanup();
        // Only report benchmark figures if the test passed
        if (testPassed && QBenchmarkTestMethodData::current->resultsAccepted())
            QTestLog::addBenchmarkResult(qMedian(results));
    }
}

/*!
    \internal

    Call slot_data(), init(), slot(), cleanup(), init(), slot(), cleanup(), ...
    If data is set then it is the only test that is performed

    If the function was successfully called, true is returned, otherwise
    false.
 */
static bool qInvokeTestMethod(const char *slotName, const char *data=0)
{
    QTEST_ASSERT(slotName);

    QBenchmarkTestMethodData benchmarkData;
    QBenchmarkTestMethodData::current = &benchmarkData;

    QBenchmarkGlobalData::current->context.slotName = QLatin1String(slotName);

    char member[512];
    QTestTable table;

    char *slot = qstrdup(slotName);
    slot[strlen(slot) - 2] = '\0';
    QTestResult::setCurrentTestFunction(slot);

    const QTestTable *gTable = QTestTable::globalTestTable();
    const int globalDataCount = gTable->dataCount();
    int curGlobalDataIndex = 0;

    /* For each test function that has a *_data() table/function, do: */
    do {
        if (!gTable->isEmpty())
            QTestResult::setCurrentGlobalTestData(gTable->testData(curGlobalDataIndex));

        if (curGlobalDataIndex == 0) {
            qsnprintf(member, 512, "%s_data()", slot);
            invokeMethod(QTest::currentTestObject, member);
        }

        bool foundFunction = false;
        if (!QTestResult::skipCurrentTest()) {
            int curDataIndex = 0;
            const int dataCount = table.dataCount();

            // Data tag requested but none available?
            if (data && !dataCount) {
                // Let empty data tag through.
                if (!*data)
                    data = 0;
                else {
                    fprintf(stderr, "Unknown testdata for function %s: '%s'\n", slotName, data);
                    fprintf(stderr, "Function has no testdata.\n");
                    return false;
                }
            }

            /* For each entry in the data table, do: */
            do {
                QTestResult::setSkipCurrentTest(false);
                if (!data || !qstrcmp(data, table.testData(curDataIndex)->dataTag())) {
                    foundFunction = true;

                    QTestPrivate::checkBlackList(slot, dataCount ? table.testData(curDataIndex)->dataTag() : 0);

                    QTestDataSetter s(curDataIndex >= dataCount ? static_cast<QTestData *>(0)
                                                      : table.testData(curDataIndex));

                    qInvokeTestMethodDataEntry(slot);

                    if (data)
                        break;
                }
                ++curDataIndex;
            } while (curDataIndex < dataCount);
        }

        if (data && !foundFunction) {
            fprintf(stderr, "Unknown testdata for function %s: '%s'\n", slotName, data);
            fprintf(stderr, "Available testdata:\n");
            for (int i = 0; i < table.dataCount(); ++i)
                fprintf(stderr, "%s\n", table.testData(i)->dataTag());
            return false;
        }

        QTestResult::setCurrentGlobalTestData(0);
        ++curGlobalDataIndex;
    } while (curGlobalDataIndex < globalDataCount);

    QTestResult::finishedCurrentTestFunction();
    QTestResult::setSkipCurrentTest(false);
    QTestResult::setCurrentTestData(0);
    delete[] slot;

    return true;
}

void *fetchData(QTestData *data, const char *tagName, int typeId)
{
    QTEST_ASSERT(typeId);
    QTEST_ASSERT_X(data, "QTest::fetchData()", "Test data requested, but no testdata available.");
    QTEST_ASSERT(data->parent());

    int idx = data->parent()->indexOf(tagName);

    if (idx == -1 || idx >= data->dataCount()) {
        qFatal("QFETCH: Requested testdata '%s' not available, check your _data function.",
                tagName);
    }

    if (typeId != data->parent()->elementTypeId(idx)) {
        qFatal("Requested type '%s' does not match available type '%s'.",
               QMetaType::typeName(typeId),
               QMetaType::typeName(data->parent()->elementTypeId(idx)));
    }

    return data->data(idx);
}

static char toHex(ushort value)
{
    static const char hexdigits[] = "0123456789ABCDEF";
    return hexdigits[value & 0xF];
}

/*!
  \fn char* QTest::toHexRepresentation(const char *ba, int length)

  Returns a pointer to a string that is the string \a ba represented
  as a space-separated sequence of hex characters. If the input is
  considered too long, it is truncated. A trucation is indicated in
  the returned string as an ellipsis at the end.

  \a length is the length of the string \a ba.
 */
char *toHexRepresentation(const char *ba, int length)
{
    if (length == 0)
        return qstrdup("");

    /* We output at maximum about maxLen characters in order to avoid
     * running out of memory and flooding things when the byte array
     * is large.
     *
     * maxLen can't be for example 200 because Qt Test is sprinkled with fixed
     * size char arrays.
     * */
    const int maxLen = 50;
    const int len = qMin(maxLen, length);
    char *result = 0;

    if (length > maxLen) {
        const int size = len * 3 + 4;
        result = new char[size];

        char *const forElipsis = result + size - 5;
        forElipsis[0] = ' ';
        forElipsis[1] = '.';
        forElipsis[2] = '.';
        forElipsis[3] = '.';
        result[size - 1] = '\0';
    }
    else {
        const int size = len * 3;
        result = new char[size];
        result[size - 1] = '\0';
    }

    int i = 0;
    int o = 0;

    while (true) {
        const char at = ba[i];

        result[o] = toHex(at >> 4);
        ++o;
        result[o] = toHex(at);

        ++i;
        ++o;
        if (i == len)
            break;
        else {
            result[o] = ' ';
            ++o;
        }
    }

    return result;
}

/*!
    \internal
    Returns the same QString but with only the ASCII characters still shown;
    everything else is replaced with \c {\uXXXX}.
*/
char *toPrettyUnicode(const ushort *p, int length)
{
    // keep it simple for the vast majority of cases
    bool trimmed = false;
    QScopedArrayPointer<char> buffer(new char[256]);
    const ushort *end = p + length;
    char *dst = buffer.data();

    *dst++ = '"';
    for ( ; p != end; ++p) {
        if (dst - buffer.data() > 245) {
            // plus the the quote, the three dots and NUL, it's 250, 251 or 255
            trimmed = true;
            break;
        }

        if (*p < 0x7f && *p >= 0x20 && *p != '\\' && *p != '"') {
            *dst++ = *p;
            continue;
        }

        // write as an escape sequence
        // this means we may advance dst to buffer.data() + 246 or 250
        *dst++ = '\\';
        switch (*p) {
        case 0x22:
        case 0x5c:
            *dst++ = uchar(*p);
            break;
        case 0x8:
            *dst++ = 'b';
            break;
        case 0xc:
            *dst++ = 'f';
            break;
        case 0xa:
            *dst++ = 'n';
            break;
        case 0xd:
            *dst++ = 'r';
            break;
        case 0x9:
            *dst++ = 't';
            break;
        default:
            *dst++ = 'u';
            *dst++ = toHex(*p >> 12);
            *dst++ = toHex(*p >> 8);
            *dst++ = toHex(*p >> 4);
            *dst++ = toHex(*p);
        }
    }

    *dst++ = '"';
    if (trimmed) {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = '\0';
    return buffer.take();
}

static void qInvokeTestMethods(QObject *testObject)
{
    const QMetaObject *metaObject = testObject->metaObject();
    QTEST_ASSERT(metaObject);
    QTestLog::startLogging();
    QTestResult::setCurrentTestFunction("initTestCase");
    QTestTable::globalTestTable();
    invokeMethod(testObject, "initTestCase_data()");

    if (!QTestResult::skipCurrentTest() && !QTest::currentTestFailed()) {
        invokeMethod(testObject, "initTestCase()");

        // finishedCurrentTestDataCleanup() resets QTestResult::currentTestFailed(), so use a local copy.
        const bool previousFailed = QTestResult::currentTestFailed();
        QTestResult::finishedCurrentTestData();
        QTestResult::finishedCurrentTestDataCleanup();
        QTestResult::finishedCurrentTestFunction();

        if (!QTestResult::skipCurrentTest() && !previousFailed) {

            if (QTest::testFuncs) {
                for (int i = 0; i != QTest::testFuncCount; i++) {
                    if (!qInvokeTestMethod(metaObject->method(QTest::testFuncs[i].function()).methodSignature().constData(),
                                                              QTest::testFuncs[i].data())) {
                        break;
                    }
                }
                testFuncCleaner.cleanup();
            } else {
                int methodCount = metaObject->methodCount();
                QMetaMethod *testMethods = new QMetaMethod[methodCount];
                for (int i = 0; i != methodCount; i++)
                    testMethods[i] = metaObject->method(i);
                for (int i = 0; i != methodCount; i++) {
                    if (!isValidSlot(testMethods[i]))
                        continue;
                    if (!qInvokeTestMethod(testMethods[i].methodSignature().constData()))
                        break;
                }
                delete[] testMethods;
                testMethods = 0;
            }
        }

        QTestResult::setSkipCurrentTest(false);
        QTestResult::setCurrentTestFunction("cleanupTestCase");
        invokeMethod(testObject, "cleanupTestCase()");
        QTestResult::finishedCurrentTestData();
        QTestResult::finishedCurrentTestDataCleanup();
    }
    QTestResult::finishedCurrentTestFunction();
    QTestResult::setCurrentTestFunction(0);
    QTestTable::clearGlobalTestTable();

    QTestLog::stopLogging();
}

#if defined(Q_OS_UNIX)
class FatalSignalHandler
{
public:
    FatalSignalHandler();
    ~FatalSignalHandler();

private:
    static void signal(int);
    sigset_t handledSignals;
};

void FatalSignalHandler::signal(int signum)
{
    qFatal("Received signal %d", signum);
#if defined(Q_OS_INTEGRITY)
    {
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = SIG_DFL;
        sigaction(signum, &act, NULL);
    }
#endif
}

FatalSignalHandler::FatalSignalHandler()
{
    sigemptyset(&handledSignals);

    const int fatalSignals[] = {
         SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGFPE, SIGSEGV, SIGPIPE, SIGTERM, 0 };

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = FatalSignalHandler::signal;

    // Remove the handler after it is invoked.
#if !defined(Q_OS_INTEGRITY)
    act.sa_flags = SA_RESETHAND;
#endif
    // Block all fatal signals in our signal handler so we don't try to close
    // the testlog twice.
    sigemptyset(&act.sa_mask);
    for (int i = 0; fatalSignals[i]; ++i)
        sigaddset(&act.sa_mask, fatalSignals[i]);

    struct sigaction oldact;

    for (int i = 0; fatalSignals[i]; ++i) {
        sigaction(fatalSignals[i], &act, &oldact);
        if (
#ifdef SA_SIGINFO
            oldact.sa_flags & SA_SIGINFO ||
#endif
            oldact.sa_handler != SIG_DFL) {
            sigaction(fatalSignals[i], &oldact, 0);
        } else
        {
            sigaddset(&handledSignals, fatalSignals[i]);
        }
    }
}


FatalSignalHandler::~FatalSignalHandler()
{
    // Unregister any of our remaining signal handlers
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;

    struct sigaction oldact;

    for (int i = 1; i < 32; ++i) {
        if (!sigismember(&handledSignals, i))
            continue;
        sigaction(i, &act, &oldact);

        // If someone overwrote it in the mean time, put it back
        if (oldact.sa_handler != FatalSignalHandler::signal)
            sigaction(i, &oldact, 0);
    }
}

#endif


} // namespace

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
static LONG WINAPI windowsFaultHandler(struct _EXCEPTION_POINTERS *exInfo)
{
    char appName[MAX_PATH];
    if (!GetModuleFileNameA(NULL, appName, MAX_PATH))
        appName[0] = 0;
    fprintf(stderr, "A crash occurred in %s (exception code 0x%lx).",
            appName, exInfo->ExceptionRecord->ExceptionCode);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif // Q_OS_WIN) && !Q_OS_WINCE && !Q_OS_WINRT

static void initEnvironment()
{
    qputenv("QT_LOGGING_TO_CONSOLE", "1");
    qputenv("QT_QTESTLIB_RUNNING", "1");
}

/*!
    Executes tests declared in \a testObject. In addition, the private slots
    \c{initTestCase()}, \c{cleanupTestCase()}, \c{init()} and \c{cleanup()}
    are executed if they exist. See \l{Creating a Test} for more details.

    Optionally, the command line arguments \a argc and \a argv can be provided.
    For a list of recognized arguments, read \l {Qt Test Command Line Arguments}.

    The following example will run all tests in \c MyTestObject:

    \snippet code/src_qtestlib_qtestcase.cpp 18

    This function returns 0 if no tests failed, or a value other than 0 if one
    or more tests failed or in case of unhandled exceptions.  (Skipped tests do
    not influence the return value.)

    For stand-alone test applications, the convenience macro \l QTEST_MAIN() can
    be used to declare a main() function that parses the command line arguments
    and executes the tests, avoiding the need to call this function explicitly.

    The return value from this function is also the exit code of the test
    application when the \l QTEST_MAIN() macro is used.

    For stand-alone test applications, this function should not be called more
    than once, as command-line options for logging test output to files and
    executing individual test functions will not behave correctly.

    Note: This function is not reentrant, only one test can run at a time. A
    test that was executed with qExec() can't run another test via qExec() and
    threads are not allowed to call qExec() simultaneously.

    If you have programatically created the arguments, as opposed to getting them
    from the arguments in \c main(), it is likely of interest to use
    QTest::qExec(QObject *, const QStringList &) since it is Unicode safe.

    \sa QTEST_MAIN()
*/

int QTest::qExec(QObject *testObject, int argc, char **argv)
{
    initEnvironment();
    QBenchmarkGlobalData benchmarkData;
    QBenchmarkGlobalData::current = &benchmarkData;

#ifdef QTESTLIB_USE_VALGRIND
    int callgrindChildExitCode = 0;
#endif

#if defined(Q_OS_MACX)
    bool macNeedsActivate = qApp && (qstrcmp(qApp->metaObject()->className(), "QApplication") == 0);
    IOPMAssertionID powerID;
#endif
#ifndef QT_NO_EXCEPTIONS
    try {
#endif

#if defined(Q_OS_MACX)
    if (macNeedsActivate) {
        CFStringRef reasonForActivity= CFSTR("No Display Sleep");
        IOReturn ok = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn, reasonForActivity, &powerID);

        if (ok != kIOReturnSuccess)
            macNeedsActivate = false; // no need to release the assertion on exit.
    }
#endif

    QTestPrivate::parseBlackList();

    QTestResult::reset();

    QTEST_ASSERT(testObject);
    QTEST_ASSERT(!currentTestObject);
    currentTestObject = testObject;

    const QMetaObject *metaObject = testObject->metaObject();
    QTEST_ASSERT(metaObject);

    QTestResult::setCurrentTestObject(metaObject->className());
    if (argc > 0)
        QTestResult::setCurrentAppName(argv[0]);

    qtest_qParseArgs(argc, argv, false);

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    if (!noCrashHandler) {
# ifndef Q_CC_MINGW
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
# endif
# ifndef Q_OS_WINRT
        SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
        SetUnhandledExceptionFilter(windowsFaultHandler);
# endif
    } // !noCrashHandler
#endif // Q_OS_WIN) && !Q_OS_WINCE && !Q_OS_WINRT

#ifdef QTESTLIB_USE_VALGRIND
    if (QBenchmarkGlobalData::current->mode() == QBenchmarkGlobalData::CallgrindParentProcess) {
        if (!qApp)
            qFatal("QtTest: -callgrind option is not available with QTEST_APPLESS_MAIN");

        const QStringList origAppArgs(QCoreApplication::arguments());
        if (!QBenchmarkValgrindUtils::rerunThroughCallgrind(origAppArgs, callgrindChildExitCode))
            return -1;

        QBenchmarkValgrindUtils::cleanup();

    } else
#endif
    {
#if defined(Q_OS_UNIX)
        QScopedPointer<FatalSignalHandler> handler;
        if (!noCrashHandler)
            handler.reset(new FatalSignalHandler);
#endif
        qInvokeTestMethods(testObject);
    }

#ifndef QT_NO_EXCEPTIONS
     } catch (...) {
         QTestResult::addFailure("Caught unhandled exception", __FILE__, __LINE__);
         if (QTestResult::currentTestFunction()) {
             QTestResult::finishedCurrentTestFunction();
             QTestResult::setCurrentTestFunction(0);
         }

        QTestLog::stopLogging();
#if defined(Q_OS_MACX)
         if (macNeedsActivate) {
             IOPMAssertionRelease(powerID);
         }
#endif
         currentTestObject = 0;

         // Rethrow exception to make debugging easier.
         throw;
         return 1;
     }
#endif

    currentTestObject = 0;

    QSignalDumper::endDump();

#if defined(Q_OS_MACX)
     if (macNeedsActivate) {
         IOPMAssertionRelease(powerID);
     }
#endif

#ifdef QTESTLIB_USE_VALGRIND
    if (QBenchmarkGlobalData::current->mode() == QBenchmarkGlobalData::CallgrindParentProcess)
        return callgrindChildExitCode;
#endif
    // make sure our exit code is never going above 127
    // since that could wrap and indicate 0 test fails
    return qMin(QTestLog::failCount(), 127);
}

/*!
  \overload
  \since 4.4

  Behaves identically to qExec(QObject *, int, char**) but takes a
  QStringList of \a arguments instead of a \c char** list.
 */
int QTest::qExec(QObject *testObject, const QStringList &arguments)
{
    const int argc = arguments.count();
    QVarLengthArray<char *> argv(argc);

    QVector<QByteArray> args;
    args.reserve(argc);

    for (int i = 0; i < argc; ++i)
    {
        args.append(arguments.at(i).toLocal8Bit().constData());
        argv[i] = args.last().data();
    }

    return qExec(testObject, argc, argv.data());
}

/*! \internal
 */
void QTest::qFail(const char *statementStr, const char *file, int line)
{
    QTestResult::addFailure(statementStr, file, line);
}

/*! \internal
 */
bool QTest::qVerify(bool statement, const char *statementStr, const char *description,
                   const char *file, int line)
{
    return QTestResult::verify(statement, statementStr, description, file, line);
}

/*! \fn void QTest::qSkip(const char *message, const char *file, int line)
\internal
 */
void QTest::qSkip(const char *message, const char *file, int line)
{
    QTestResult::addSkip(message, file, line);
    QTestResult::setSkipCurrentTest(true);
}

/*! \fn bool QTest::qExpectFail(const char *dataIndex, const char *comment, TestFailMode mode, const char *file, int line)
\internal
 */
bool QTest::qExpectFail(const char *dataIndex, const char *comment,
                       QTest::TestFailMode mode, const char *file, int line)
{
    return QTestResult::expectFail(dataIndex, qstrdup(comment), mode, file, line);
}

/*! \internal
 */
void QTest::qWarn(const char *message, const char *file, int line)
{
    QTestLog::warn(message, file, line);
}

/*!
    Ignores messages created by qDebug() or qWarning(). If the \a message
    with the corresponding \a type is outputted, it will be removed from the
    test log. If the test finished and the \a message was not outputted,
    a test failure is appended to the test log.

    \b {Note:} Invoking this function will only ignore one message.
    If the message you want to ignore is outputted twice, you have to
    call ignoreMessage() twice, too.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 19

    The example above tests that QDir::mkdir() outputs the right warning when invoked
    with an invalid file name.
*/
void QTest::ignoreMessage(QtMsgType type, const char *message)
{
    QTestLog::ignoreMessage(type, message);
}

#ifndef QT_NO_REGULAREXPRESSION
/*!
    \overload

    Ignores messages created by qDebug() or qWarning(). If the message
    matching \a messagePattern
    with the corresponding \a type is outputted, it will be removed from the
    test log. If the test finished and the message was not outputted,
    a test failure is appended to the test log.

    \b {Note:} Invoking this function will only ignore one message.
    If the message you want to ignore is outputted twice, you have to
    call ignoreMessage() twice, too.

    \since 5.3
*/
void QTest::ignoreMessage(QtMsgType type, const QRegularExpression &messagePattern)
{
    QTestLog::ignoreMessage(type, messagePattern);
}
#endif // QT_NO_REGULAREXPRESSION

/*! \internal
 */

#ifdef Q_OS_WIN
static inline bool isWindowsBuildDirectory(const QString &dirName)
{
    return dirName.compare(QLatin1String("Debug"), Qt::CaseInsensitive) == 0
           || dirName.compare(QLatin1String("Release"), Qt::CaseInsensitive) == 0;
}
#endif

/*! \internal
 */

QString QTest::qFindTestData(const QString& base, const char *file, int line, const char *builddir)
{
    QString found;

    // Testdata priorities:

    //  1. relative to test binary.
    if (qApp) {
        QDir binDirectory(QCoreApplication::applicationDirPath());
        if (binDirectory.exists(base)) {
            found = binDirectory.absoluteFilePath(base);
        }
#ifdef Q_OS_WIN
        // Windows: The executable is typically located in one of the
        // 'Release' or 'Debug' directories.
        else if (isWindowsBuildDirectory(binDirectory.dirName())
                 && binDirectory.cdUp() && binDirectory.exists(base)) {
            found = binDirectory.absoluteFilePath(base);
        }
#endif // Q_OS_WIN
        else if (QTestLog::verboseLevel() >= 2) {
            const QString candidate = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QLatin1Char('/') + base);
            QTestLog::info(qPrintable(
                QString::fromLatin1("testdata %1 not found relative to test binary [%2]; "
                                    "checking next location").arg(base, candidate)),
                file, line);
        }
    }

    //  2. installed path.
    if (found.isEmpty()) {
        const char *testObjectName = QTestResult::currentTestObjectName();
        if (testObjectName) {
            QString testsPath = QLibraryInfo::location(QLibraryInfo::TestsPath);
            QString candidate = QString::fromLatin1("%1/%2/%3")
                .arg(testsPath, QFile::decodeName(testObjectName).toLower(), base);
            if (QFileInfo(candidate).exists()) {
                found = candidate;
            }
            else if (QTestLog::verboseLevel() >= 2) {
                QTestLog::info(qPrintable(
                    QString::fromLatin1("testdata %1 not found in tests install path [%2]; "
                                        "checking next location")
                        .arg(base, QDir::toNativeSeparators(candidate))),
                    file, line);
            }
        }
    }

    //  3. relative to test source.
    if (found.isEmpty()) {
        // srcdir is the directory containing the calling source file.
        QFileInfo srcdir = QFileInfo(QFile::decodeName(file)).path();

        // If the srcdir is relative, that means it is relative to the current working
        // directory of the compiler at compile time, which should be passed in as `builddir'.
        if (!srcdir.isAbsolute() && builddir) {
            srcdir.setFile(QFile::decodeName(builddir) + QLatin1String("/") + srcdir.filePath());
        }

        QString candidate = QString::fromLatin1("%1/%2").arg(srcdir.canonicalFilePath(), base);
        if (QFileInfo(candidate).exists()) {
            found = candidate;
        }
        else if (QTestLog::verboseLevel() >= 2) {
            QTestLog::info(qPrintable(
                QString::fromLatin1("testdata %1 not found relative to source path [%2]")
                    .arg(base, QDir::toNativeSeparators(candidate))),
                file, line);
        }
    }

    // 4. Try resources
    if (found.isEmpty()) {
        QString candidate = QString::fromLatin1(":/%1").arg(base);
        if (QFileInfo(candidate).exists())
            found = candidate;
    }

    // 5. Try current directory
    if (found.isEmpty()) {
        QString candidate = QString::fromLatin1("%1/%2").arg(QDir::currentPath()).arg(base);
        if (QFileInfo(candidate).exists())
            found = candidate;
    }

    if (found.isEmpty()) {
        QTest::qWarn(qPrintable(
            QString::fromLatin1("testdata %1 could not be located!").arg(base)),
            file, line);
    }
    else if (QTestLog::verboseLevel() >= 1) {
        QTestLog::info(qPrintable(
            QString::fromLatin1("testdata %1 was located at %2").arg(base, QDir::toNativeSeparators(found))),
            file, line);
    }

    return found;
}

/*! \internal
 */
QString QTest::qFindTestData(const char *base, const char *file, int line, const char *builddir)
{
    return qFindTestData(QFile::decodeName(base), file, line, builddir);
}

/*! \internal
 */
void *QTest::qData(const char *tagName, int typeId)
{
    return fetchData(QTestResult::currentTestData(), tagName, typeId);
}

/*! \internal
 */
void *QTest::qGlobalData(const char *tagName, int typeId)
{
    return fetchData(QTestResult::currentGlobalTestData(), tagName, typeId);
}

/*! \internal
 */
void *QTest::qElementData(const char *tagName, int metaTypeId)
{
    QTEST_ASSERT(tagName);
    QTestData *data = QTestResult::currentTestData();
    QTEST_ASSERT(data);
    QTEST_ASSERT(data->parent());

    int idx = data->parent()->indexOf(tagName);
    QTEST_ASSERT(idx != -1);
    QTEST_ASSERT(data->parent()->elementTypeId(idx) == metaTypeId);

    return data->data(data->parent()->indexOf(tagName));
}

/*! \internal
 */
void QTest::addColumnInternal(int id, const char *name)
{
    QTestTable *tbl = QTestTable::currentTestTable();
    QTEST_ASSERT_X(tbl, "QTest::addColumn()", "Cannot add testdata outside of a _data slot.");

    tbl->addColumn(id, name);
}

/*!
    Appends a new row to the current test data. \a dataTag is the name of
    the testdata that will appear in the test output. Returns a QTestData reference
    that can be used to stream in data.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 20

    \b {Note:} This macro can only be used in a test's data function
    that is invoked by the test framework.

    See \l {Chapter 2: Data Driven Testing}{Data Driven Testing} for
    a more extensive example.

    \sa addColumn(), QFETCH()
*/
QTestData &QTest::newRow(const char *dataTag)
{
    QTEST_ASSERT_X(dataTag, "QTest::newRow()", "Data tag can not be null");
    QTestTable *tbl = QTestTable::currentTestTable();
    QTEST_ASSERT_X(tbl, "QTest::newRow()", "Cannot add testdata outside of a _data slot.");
    QTEST_ASSERT_X(tbl->elementCount(), "QTest::newRow()", "Must add columns before attempting to add rows.");

    return *tbl->newData(dataTag);
}

/*! \fn void QTest::addColumn(const char *name, T *dummy = 0)

    Adds a column with type \c{T} to the current test data.
    \a name is the name of the column. \a dummy is a workaround
    for buggy compilers and can be ignored.

    To populate the column with values, newRow() can be used. Use
    \l QFETCH() to fetch the data in the actual test.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 21

    To add custom types to the testdata, the type must be registered with
    QMetaType via \l Q_DECLARE_METATYPE().

    \b {Note:} This macro can only be used in a test's data function
    that is invoked by the test framework.

    See \l {Chapter 2: Data Driven Testing}{Data Driven Testing} for
    a more extensive example.

    \sa QTest::newRow(), QFETCH(), QMetaType
*/

/*!
    Returns the name of the binary that is currently executed.
*/
const char *QTest::currentAppName()
{
    return QTestResult::currentAppName();
}

/*!
    Returns the name of the test function that is currently executed.

    Example:

    \snippet code/src_qtestlib_qtestcase.cpp 22
*/
const char *QTest::currentTestFunction()
{
    return QTestResult::currentTestFunction();
}

/*!
    Returns the name of the current test data. If the test doesn't
    have any assigned testdata, the function returns 0.
*/
const char *QTest::currentDataTag()
{
    return QTestResult::currentDataTag();
}

/*!
    Returns \c true if the current test function failed, otherwise false.
*/
bool QTest::currentTestFailed()
{
    return QTestResult::currentTestFailed();
}

/*!
    Sleeps for \a ms milliseconds, blocking execution of the
    test. qSleep() will not do any event processing and leave your test
    unresponsive. Network communication might time out while
    sleeping. Use \l qWait() to do non-blocking sleeping.

    \a ms must be greater than 0.

    \b {Note:} The qSleep() function calls either \c nanosleep() on
    unix or \c Sleep() on windows, so the accuracy of time spent in
    qSleep() depends on the operating system.

    Example:
    \snippet code/src_qtestlib_qtestcase.cpp 23

    \sa qWait()
*/
void QTest::qSleep(int ms)
{
    QTEST_ASSERT(ms > 0);

#if defined(Q_OS_WINRT)
    WaitForSingleObjectEx(GetCurrentThread(), ms, true);
#elif defined(Q_OS_WIN)
    Sleep(uint(ms));
#else
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
#endif
}

/*! \internal
 */
QObject *QTest::testObject()
{
    return currentTestObject;
}

/*! \internal
    This function is called by various specializations of QTest::qCompare
    to decide whether to report a failure and to produce verbose test output.

    The failureMsg parameter can be null, in which case a default message
    will be output if the compare fails.  If the compare succeeds, failureMsg
    will not be output.

    If the caller has already passed a failure message showing the compared
    values, or if those values cannot be stringified, val1 and val2 can be null.
 */
bool QTest::compare_helper(bool success, const char *failureMsg,
                           char *val1, char *val2,
                           const char *actual, const char *expected,
                           const char *file, int line)
{
    return QTestResult::compare(success, failureMsg, val1, val2, actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(float const &t1, float const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
 */
bool QTest::qCompare(float const &t1, float const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    return compare_helper(qFuzzyCompare(t1, t2), "Compared floats are not the same (fuzzy compare)",
                          toString(t1), toString(t2), actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(double const &t1, double const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
 */
bool QTest::qCompare(double const &t1, double const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    return compare_helper(qFuzzyCompare(t1, t2), "Compared doubles are not the same (fuzzy compare)",
                          toString(t1), toString(t2), actual, expected, file, line);
}

/*! \fn bool QTest::qCompare(double const &t1, float const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
 */

/*! \fn bool QTest::qCompare(float const &t1, double const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
 */

#define TO_STRING_IMPL(TYPE, FORMAT) \
template <> Q_TESTLIB_EXPORT char *QTest::toString<TYPE >(const TYPE &t) \
{ \
    char *msg = new char[128]; \
    qsnprintf(msg, 128, #FORMAT, t); \
    return msg; \
}

TO_STRING_IMPL(short, %hd)
TO_STRING_IMPL(ushort, %hu)
TO_STRING_IMPL(int, %d)
TO_STRING_IMPL(uint, %u)
TO_STRING_IMPL(long, %ld)
TO_STRING_IMPL(ulong, %lu)
#if defined(Q_OS_WIN)
TO_STRING_IMPL(qint64, %I64d)
TO_STRING_IMPL(quint64, %I64u)
#else
TO_STRING_IMPL(qint64, %lld)
TO_STRING_IMPL(quint64, %llu)
#endif
TO_STRING_IMPL(bool, %d)
TO_STRING_IMPL(char, %c)
TO_STRING_IMPL(float, %g)
TO_STRING_IMPL(double, %lg)

/*! \internal
 */
char *QTest::toString(const char *str)
{
    if (!str)
        return 0;
    char *msg = new char[strlen(str) + 1];
    return qstrcpy(msg, str);
}

/*! \internal
 */
char *QTest::toString(const void *p)
{
    char *msg = new char[128];
    qsnprintf(msg, 128, "%p", p);
    return msg;
}

/*! \internal
 */
bool QTest::compare_string_helper(const char *t1, const char *t2, const char *actual,
                                  const char *expected, const char *file, int line)
{
    return compare_helper(qstrcmp(t1, t2) == 0, "Compared strings are not the same",
                          toString(t1), toString(t2), actual, expected, file, line);
}

/*! \fn bool QTest::compare_ptr_helper(const void *t1, const void *t2, const char *actual, const char *expected, const char *file, int line);
    \internal
*/

/*! \fn bool QTest::qCompare(T1 const &, T2 const &, const char *, const char *, const char *, int);
    \internal
*/

/*! \fn bool QTest::qCompare(QIcon const &t1, QIcon const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(QImage const &t1, QImage const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(QPixmap const &t1, QPixmap const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(T const &t1, T const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const T *t1, const T *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(T *t1, T *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const T1 *t1, const T2 *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(T1 *t1, T2 *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const char *t1, const char *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(char *t1, char *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(char *t1, const char *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(const char *t1, char *t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(QString const &t1, QLatin1String const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(QLatin1String const &t1, QString const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(QStringList const &t1, QStringList const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(QList<T> const &t1, QList<T> const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(QFlags<T> const &t1, T const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(QFlags<T> const &t1, int const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(qint64 const &t1, qint32 const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(qint64 const &t1, quint32 const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(quint64 const &t1, quint32 const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(qint32 const &t1, qint64 const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(quint32 const &t1, qint64 const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(quint32 const &t1, quint64 const &t2, const char *actual, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn bool QTest::qCompare(bool const &t1, int const &t2, const char *actual, const char *expected, const char *file, int line)
  \internal
 */

/*! \fn bool QTest::qTest(const T& actual, const char *elementName, const char *actualStr, const char *expected, const char *file, int line)
    \internal
*/

/*! \fn void QTest::sendKeyEvent(KeyAction action, QWidget *widget, Qt::Key code, QString text, Qt::KeyboardModifiers modifier, int delay=-1)
    \internal
*/

/*! \fn void QTest::sendKeyEvent(KeyAction action, QWindow *window, Qt::Key code, QString text, Qt::KeyboardModifiers modifier, int delay=-1)
    \internal
*/

/*! \fn void QTest::sendKeyEvent(KeyAction action, QWidget *widget, Qt::Key code, char ascii, Qt::KeyboardModifiers modifier, int delay=-1)
    \internal
*/

/*! \fn void QTest::sendKeyEvent(KeyAction action, QWindow *window, Qt::Key code, char ascii, Qt::KeyboardModifiers modifier, int delay=-1)
    \internal
*/

/*! \fn void QTest::simulateEvent(QWidget *widget, bool press, int code, Qt::KeyboardModifiers modifier, QString text, bool repeat, int delay=-1)
    \internal
*/

/*! \fn void QTest::simulateEvent(QWindow *window, bool press, int code, Qt::KeyboardModifiers modifier, QString text, bool repeat, int delay=-1)
    \internal
*/

QT_END_NAMESPACE
