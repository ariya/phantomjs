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

#ifndef QTESTCASE_H
#define QTESTCASE_H

#include <QtTest/qtest_global.h>

#include <QtCore/qstring.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qtypetraits.h>

#include <string.h>

#ifndef QT_NO_EXCEPTIONS
#  include <exception>
#endif // QT_NO_EXCEPTIONS


QT_BEGIN_NAMESPACE

class QRegularExpression;

#define QVERIFY(statement) \
do {\
    if (!QTest::qVerify((statement), #statement, "", __FILE__, __LINE__))\
        return;\
} while (0)

#define QFAIL(message) \
do {\
    QTest::qFail(message, __FILE__, __LINE__);\
    return;\
} while (0)

#define QVERIFY2(statement, description) \
do {\
    if (statement) {\
        if (!QTest::qVerify(true, #statement, (description), __FILE__, __LINE__))\
            return;\
    } else {\
        if (!QTest::qVerify(false, #statement, (description), __FILE__, __LINE__))\
            return;\
    }\
} while (0)

#define QCOMPARE(actual, expected) \
do {\
    if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__))\
        return;\
} while (0)


#ifndef QT_NO_EXCEPTIONS

#  define QVERIFY_EXCEPTION_THROWN(expression, exceptiontype) \
    do {\
        QT_TRY {\
            QT_TRY {\
                expression;\
                QTest::qFail("Expected exception of type " #exceptiontype " to be thrown" \
                             " but no exception caught", __FILE__, __LINE__);\
                return;\
            } QT_CATCH (const exceptiontype &) {\
            }\
        } QT_CATCH (const std::exception &e) {\
            QByteArray msg = QByteArray() + "Expected exception of type " #exceptiontype \
                             " to be thrown but std::exception caught with message: " + e.what(); \
            QTest::qFail(msg.constData(), __FILE__, __LINE__);\
            return;\
        } QT_CATCH (...) {\
            QTest::qFail("Expected exception of type " #exceptiontype " to be thrown" \
                         " but unknown exception caught", __FILE__, __LINE__);\
            return;\
        }\
    } while (0)

#else // QT_NO_EXCEPTIONS

/*
 * The expression passed to the macro should throw an exception and we can't
 * catch it because Qt has been compiled without exception support. We can't
 * skip the expression because it may have side effects and must be executed.
 * So, users must use Qt with exception support enabled if they use exceptions
 * in their code.
 */
#  define QVERIFY_EXCEPTION_THROWN(expression, exceptiontype) \
    Q_STATIC_ASSERT_X(false, "Support of exceptions is disabled")

#endif // !QT_NO_EXCEPTIONS


#define QTRY_LOOP_IMPL(__expr, __timeoutValue, __step) \
    if (!(__expr)) { \
        QTest::qWait(0); \
    } \
    int __i = 0; \
    for (; __i < __timeoutValue && !(__expr); __i += __step) { \
        QTest::qWait(__step); \
    }

#define QTRY_TIMEOUT_DEBUG_IMPL(__expr, __timeoutValue, __step)\
    if (!(__expr)) { \
        QTRY_LOOP_IMPL((__expr), (2 * __timeoutValue), __step);\
        if (__expr) { \
            QString msg = QString::fromUtf8("QTestLib: This test case check (\"%1\") failed because the requested timeout (%2 ms) was too short, %3 ms would have been sufficient this time."); \
            msg = msg.arg(QString::fromUtf8(#__expr)).arg(__timeoutValue).arg(__timeoutValue + __i); \
            QFAIL(qPrintable(msg)); \
        } \
    }

#define QTRY_IMPL(__expr, __timeout)\
    const int __step = 50; \
    const int __timeoutValue = __timeout; \
    QTRY_LOOP_IMPL((__expr), __timeoutValue, __step); \
    QTRY_TIMEOUT_DEBUG_IMPL((__expr), __timeoutValue, __step)\

// Will try to wait for the expression to become true while allowing event processing
#define QTRY_VERIFY_WITH_TIMEOUT(__expr, __timeout) \
do { \
    QTRY_IMPL((__expr), __timeout);\
    QVERIFY(__expr); \
} while (0)

#define QTRY_VERIFY(__expr) QTRY_VERIFY_WITH_TIMEOUT((__expr), 5000)

// Will try to wait for the comparison to become successful while allowing event processing
#define QTRY_COMPARE_WITH_TIMEOUT(__expr, __expected, __timeout) \
do { \
    QTRY_IMPL(((__expr) == (__expected)), __timeout);\
    QCOMPARE((__expr), __expected); \
} while (0)

#define QTRY_COMPARE(__expr, __expected) QTRY_COMPARE_WITH_TIMEOUT((__expr), __expected, 5000)

#define QSKIP_INTERNAL(statement) \
do {\
    QTest::qSkip(statement, __FILE__, __LINE__);\
    return;\
} while (0)

#ifdef Q_COMPILER_VARIADIC_MACROS

#define QSKIP(statement, ...) QSKIP_INTERNAL(statement)

#else

#define QSKIP(statement) QSKIP_INTERNAL(statement)

#endif

#define QEXPECT_FAIL(dataIndex, comment, mode)\
do {\
    if (!QTest::qExpectFail(dataIndex, comment, QTest::mode, __FILE__, __LINE__))\
        return;\
} while (0)

#define QFETCH(type, name)\
    type name = *static_cast<type *>(QTest::qData(#name, ::qMetaTypeId<type >()))

#define QFETCH_GLOBAL(type, name)\
    type name = *static_cast<type *>(QTest::qGlobalData(#name, ::qMetaTypeId<type >()))

#define QTEST(actual, testElement)\
do {\
    if (!QTest::qTest(actual, testElement, #actual, #testElement, __FILE__, __LINE__))\
        return;\
} while (0)

#define QWARN(msg)\
    QTest::qWarn(msg, __FILE__, __LINE__)

#ifdef QT_TESTCASE_BUILDDIR
# define QFINDTESTDATA(basepath)\
    QTest::qFindTestData(basepath, __FILE__, __LINE__, QT_TESTCASE_BUILDDIR)
#else
# define QFINDTESTDATA(basepath)\
    QTest::qFindTestData(basepath, __FILE__, __LINE__)
#endif

class QObject;
class QTestData;

#define QTEST_COMPARE_DECL(KLASS)\
    template<> Q_TESTLIB_EXPORT char *toString<KLASS >(const KLASS &);

namespace QTest
{
    template <typename T>
    inline char *toString(const T &)
    {
        return 0;
    }


    Q_TESTLIB_EXPORT char *toHexRepresentation(const char *ba, int length);
    Q_TESTLIB_EXPORT char *toPrettyUnicode(const ushort *unicode, int length);
    Q_TESTLIB_EXPORT char *toString(const char *);
    Q_TESTLIB_EXPORT char *toString(const void *);

    Q_TESTLIB_EXPORT int qExec(QObject *testObject, int argc = 0, char **argv = 0);
    Q_TESTLIB_EXPORT int qExec(QObject *testObject, const QStringList &arguments);

    Q_TESTLIB_EXPORT bool qVerify(bool statement, const char *statementStr, const char *description,
                                 const char *file, int line);
    Q_TESTLIB_EXPORT void qFail(const char *statementStr, const char *file, int line);
    Q_TESTLIB_EXPORT void qSkip(const char *message, const char *file, int line);
    Q_TESTLIB_EXPORT bool qExpectFail(const char *dataIndex, const char *comment, TestFailMode mode,
                           const char *file, int line);
    Q_TESTLIB_EXPORT void qWarn(const char *message, const char *file = 0, int line = 0);
    Q_TESTLIB_EXPORT void ignoreMessage(QtMsgType type, const char *message);
#ifndef QT_NO_REGULAREXPRESSION
    Q_TESTLIB_EXPORT void ignoreMessage(QtMsgType type, const QRegularExpression &messagePattern);
#endif

    Q_TESTLIB_EXPORT QString qFindTestData(const char* basepath, const char* file = 0, int line = 0, const char* builddir = 0);
    Q_TESTLIB_EXPORT QString qFindTestData(const QString& basepath, const char* file = 0, int line = 0, const char* builddir = 0);

    Q_TESTLIB_EXPORT void *qData(const char *tagName, int typeId);
    Q_TESTLIB_EXPORT void *qGlobalData(const char *tagName, int typeId);
    Q_TESTLIB_EXPORT void *qElementData(const char *elementName, int metaTypeId);
    Q_TESTLIB_EXPORT QObject *testObject();

    Q_TESTLIB_EXPORT const char *currentAppName();

    Q_TESTLIB_EXPORT const char *currentTestFunction();
    Q_TESTLIB_EXPORT const char *currentDataTag();
    Q_TESTLIB_EXPORT bool currentTestFailed();

    Q_TESTLIB_EXPORT Qt::Key asciiToKey(char ascii);
    Q_TESTLIB_EXPORT char keyToAscii(Qt::Key key);

    Q_TESTLIB_EXPORT bool compare_helper(bool success, const char *failureMsg,
                                         char *val1, char *val2,
                                         const char *actual, const char *expected,
                                         const char *file, int line);
    Q_TESTLIB_EXPORT void qSleep(int ms);
    Q_TESTLIB_EXPORT void addColumnInternal(int id, const char *name);

    template <typename T>
    inline void addColumn(const char *name, T * = 0)
    {
        typedef QtPrivate::is_same<T, const char*> QIsSameTConstChar;
        Q_STATIC_ASSERT_X(!QIsSameTConstChar::value, "const char* is not allowed as a test data format.");
        addColumnInternal(qMetaTypeId<T>(), name);
    }
    Q_TESTLIB_EXPORT QTestData &newRow(const char *dataTag);

    template <typename T>
    inline bool qCompare(T const &t1, T const &t2, const char *actual, const char *expected,
                        const char *file, int line)
    {
        return compare_helper(t1 == t2, "Compared values are not the same",
                              toString<T>(t1), toString<T>(t2), actual, expected, file, line);
    }

    Q_TESTLIB_EXPORT bool qCompare(float const &t1, float const &t2,
                    const char *actual, const char *expected, const char *file, int line);

    Q_TESTLIB_EXPORT bool qCompare(double const &t1, double const &t2,
                    const char *actual, const char *expected, const char *file, int line);

    inline bool compare_ptr_helper(const void *t1, const void *t2, const char *actual,
                                   const char *expected, const char *file, int line)
    {
        return compare_helper(t1 == t2, "Compared pointers are not the same",
                              toString(t1), toString(t2), actual, expected, file, line);
    }

    Q_TESTLIB_EXPORT bool compare_string_helper(const char *t1, const char *t2, const char *actual,
                                      const char *expected, const char *file, int line);

#ifndef Q_QDOC
    QTEST_COMPARE_DECL(short)
    QTEST_COMPARE_DECL(ushort)
    QTEST_COMPARE_DECL(int)
    QTEST_COMPARE_DECL(uint)
    QTEST_COMPARE_DECL(long)
    QTEST_COMPARE_DECL(ulong)
    QTEST_COMPARE_DECL(qint64)
    QTEST_COMPARE_DECL(quint64)

    QTEST_COMPARE_DECL(float)
    QTEST_COMPARE_DECL(double)
    QTEST_COMPARE_DECL(char)
    QTEST_COMPARE_DECL(bool)
#endif

    template <typename T1, typename T2>
    bool qCompare(T1 const &, T2 const &, const char *, const char *, const char *, int);

    inline bool qCompare(double const &t1, float const &t2, const char *actual,
                                 const char *expected, const char *file, int line)
    {
        return qCompare(qreal(t1), qreal(t2), actual, expected, file, line);
    }

    inline bool qCompare(float const &t1, double const &t2, const char *actual,
                                 const char *expected, const char *file, int line)
    {
        return qCompare(qreal(t1), qreal(t2), actual, expected, file, line);
    }

    template <typename T>
    inline bool qCompare(const T *t1, const T *t2, const char *actual, const char *expected,
                        const char *file, int line)
    {
        return compare_ptr_helper(t1, t2, actual, expected, file, line);
    }
    template <typename T>
    inline bool qCompare(T *t1, T *t2, const char *actual, const char *expected,
                        const char *file, int line)
    {
        return compare_ptr_helper(t1, t2, actual, expected, file, line);
    }

    template <typename T1, typename T2>
    inline bool qCompare(const T1 *t1, const T2 *t2, const char *actual, const char *expected,
                        const char *file, int line)
    {
        return compare_ptr_helper(t1, static_cast<const T1 *>(t2), actual, expected, file, line);
    }
    template <typename T1, typename T2>
    inline bool qCompare(T1 *t1, T2 *t2, const char *actual, const char *expected,
                        const char *file, int line)
    {
        return compare_ptr_helper(const_cast<const T1 *>(t1),
                static_cast<const T1 *>(const_cast<const T2 *>(t2)), actual, expected, file, line);
    }
    inline bool qCompare(const char *t1, const char *t2, const char *actual,
                                       const char *expected, const char *file, int line)
    {
        return compare_string_helper(t1, t2, actual, expected, file, line);
    }
    inline bool qCompare(char *t1, char *t2, const char *actual, const char *expected,
                        const char *file, int line)
    {
        return compare_string_helper(t1, t2, actual, expected, file, line);
    }

    /* The next two overloads are for MSVC that shows problems with implicit
       conversions
     */
    inline bool qCompare(char *t1, const char *t2, const char *actual,
                         const char *expected, const char *file, int line)
    {
        return compare_string_helper(t1, t2, actual, expected, file, line);
    }
    inline bool qCompare(const char *t1, char *t2, const char *actual,
                         const char *expected, const char *file, int line)
    {
        return compare_string_helper(t1, t2, actual, expected, file, line);
    }

    // NokiaX86 and RVCT do not like implicitly comparing bool with int
    inline bool qCompare(bool const &t1, int const &t2,
                    const char *actual, const char *expected, const char *file, int line)
    {
        return qCompare(int(t1), t2, actual, expected, file, line);
    }


    template <class T>
    inline bool qTest(const T& actual, const char *elementName, const char *actualStr,
                     const char *expected, const char *file, int line)
    {
        return qCompare(actual, *static_cast<const T *>(QTest::qElementData(elementName,
                       qMetaTypeId<T>())), actualStr, expected, file, line);
    }
}

#undef QTEST_COMPARE_DECL

QT_END_NAMESPACE

#endif
