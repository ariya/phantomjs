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

#ifndef QTEST_H
#define QTEST_H

#include <QtTest/qtest_global.h>
#include <QtTest/qtestcase.h>
#include <QtTest/qtestdata.h>
#include <QtTest/qbenchmark.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qurl.h>

#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qrect.h>


QT_BEGIN_NAMESPACE


namespace QTest
{

template<> inline char *toString(const QString &str)
{
    return QTest::toPrettyUnicode(reinterpret_cast<const ushort *>(str.constData()), str.length());
}

template<> inline char *toString(const QLatin1String &str)
{
    return toString(QString(str));
}

template<> inline char *toString(const QByteArray &ba)
{
    return QTest::toHexRepresentation(ba.constData(), ba.length());
}

#ifndef QT_NO_DATESTRING
template<> inline char *toString(const QTime &time)
{
    return time.isValid()
        ? qstrdup(qPrintable(time.toString(QLatin1String("hh:mm:ss.zzz"))))
        : qstrdup("Invalid QTime");
}

template<> inline char *toString(const QDate &date)
{
    return date.isValid()
        ? qstrdup(qPrintable(date.toString(QLatin1String("yyyy/MM/dd"))))
        : qstrdup("Invalid QDate");
}

template<> inline char *toString(const QDateTime &dateTime)
{
    return dateTime.isValid()
        ? qstrdup(qPrintable(dateTime.toString(QLatin1String("yyyy/MM/dd hh:mm:ss.zzz")) +
                             (dateTime.timeSpec() == Qt::LocalTime ? QLatin1String("[local time]") : QLatin1String("[UTC]"))))
        : qstrdup("Invalid QDateTime");
}
#endif // QT_NO_DATESTRING

template<> inline char *toString(const QChar &c)
{
    return qstrdup(qPrintable(QString::fromLatin1("QChar: '%1' (0x%2)").arg(c).arg(QString::number(static_cast<int>(c.unicode()), 16))));
}

template<> inline char *toString(const QPoint &p)
{
    return qstrdup(QString::fromLatin1("QPoint(%1,%2)").arg(p.x()).arg(p.y()).toLatin1().constData());
}

template<> inline char *toString(const QSize &s)
{
    return qstrdup(QString::fromLatin1("QSize(%1x%2)").arg(s.width()).arg(s.height()).toLatin1().constData());
}

template<> inline char *toString(const QRect &s)
{
    return qstrdup(QString::fromLatin1("QRect(%1,%2 %5x%6) (bottomright %3,%4)").arg(s.left()).arg(s.top()).arg(s.right()).arg(s.bottom()).arg(s.width()).arg(s.height()).toLatin1().constData());
}

template<> inline char *toString(const QPointF &p)
{
    return qstrdup(QString::fromLatin1("QPointF(%1,%2)").arg(p.x()).arg(p.y()).toLatin1().constData());
}

template<> inline char *toString(const QSizeF &s)
{
    return qstrdup(QString::fromLatin1("QSizeF(%1x%2)").arg(s.width()).arg(s.height()).toLatin1().constData());
}

template<> inline char *toString(const QRectF &s)
{
    return qstrdup(QString::fromLatin1("QRectF(%1,%2 %5x%6) (bottomright %3,%4)").arg(s.left()).arg(s.top()).arg(s.right()).arg(s.bottom()).arg(s.width()).arg(s.height()).toLatin1().constData());
}

template<> inline char *toString(const QUrl &uri)
{
    if (!uri.isValid())
        return qstrdup(qPrintable(QStringLiteral("Invalid URL: ") + uri.errorString()));
    return qstrdup(uri.toEncoded().constData());
}

template<> inline char *toString(const QVariant &v)
{
    QByteArray vstring("QVariant(");
    if (v.isValid()) {
        QByteArray type(v.typeName());
        if (type.isEmpty()) {
            type = QByteArray::number(v.userType());
        }
        vstring.append(type);
        if (!v.isNull()) {
            vstring.append(',');
            if (v.canConvert(QVariant::String)) {
                vstring.append(qvariant_cast<QString>(v).toLocal8Bit());
            }
            else {
                vstring.append("<value not representable as string>");
            }
        }
    }
    vstring.append(')');

    return qstrdup(vstring.constData());
}

template<>
inline bool qCompare(QString const &t1, QLatin1String const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, QString(t2), actual, expected, file, line);
}
template<>
inline bool qCompare(QLatin1String const &t1, QString const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(QString(t1), t2, actual, expected, file, line);
}

template <typename T>
inline bool qCompare(QList<T> const &t1, QList<T> const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    char msg[1024];
    msg[0] = '\0';
    bool isOk = true;
    const int actualSize = t1.count();
    const int expectedSize = t2.count();
    if (actualSize != expectedSize) {
        qsnprintf(msg, sizeof(msg), "Compared lists have different sizes.\n"
                  "   Actual   (%s) size: %d\n"
                  "   Expected (%s) size: %d", actual, actualSize, expected, expectedSize);
        isOk = false;
    }
    for (int i = 0; isOk && i < actualSize; ++i) {
        if (!(t1.at(i) == t2.at(i))) {
            char *val1 = toString(t1.at(i));
            char *val2 = toString(t2.at(i));

            qsnprintf(msg, sizeof(msg), "Compared lists differ at index %d.\n"
                      "   Actual   (%s): %s\n"
                      "   Expected (%s): %s", i, actual, val1 ? val1 : "<null>",
                      expected, val2 ? val2 : "<null>");
            isOk = false;

            delete [] val1;
            delete [] val2;
        }
    }
    return compare_helper(isOk, msg, 0, 0, actual, expected, file, line);
}

template <>
inline bool qCompare(QStringList const &t1, QStringList const &t2, const char *actual, const char *expected,
                            const char *file, int line)
{
    return qCompare<QString>(t1, t2, actual, expected, file, line);
}

template <typename T>
inline bool qCompare(QFlags<T> const &t1, T const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    return qCompare(int(t1), int(t2), actual, expected, file, line);
}

template <typename T>
inline bool qCompare(QFlags<T> const &t1, int const &t2, const char *actual, const char *expected,
                    const char *file, int line)
{
    return qCompare(int(t1), t2, actual, expected, file, line);
}

template<>
inline bool qCompare(qint64 const &t1, qint32 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, static_cast<qint64>(t2), actual, expected, file, line);
}

template<>
inline bool qCompare(qint64 const &t1, quint32 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, static_cast<qint64>(t2), actual, expected, file, line);
}

template<>
inline bool qCompare(quint64 const &t1, quint32 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(t1, static_cast<quint64>(t2), actual, expected, file, line);
}

template<>
inline bool qCompare(qint32 const &t1, qint64 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(static_cast<qint64>(t1), t2, actual, expected, file, line);
}

template<>
inline bool qCompare(quint32 const &t1, qint64 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(static_cast<qint64>(t1), t2, actual, expected, file, line);
}

template<>
inline bool qCompare(quint32 const &t1, quint64 const &t2, const char *actual,
                    const char *expected, const char *file, int line)
{
    return qCompare(static_cast<quint64>(t1), t2, actual, expected, file, line);
}

}
QT_END_NAMESPACE

#define QTEST_APPLESS_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#include <QtTest/qtestsystem.h>

#if defined(QT_WIDGETS_LIB)

#include <QtTest/qtest_gui.h>

#ifdef QT_KEYPAD_NAVIGATION
#  define QTEST_DISABLE_KEYPAD_NAVIGATION QApplication::setNavigationMode(Qt::NavigationModeNone);
#else
#  define QTEST_DISABLE_KEYPAD_NAVIGATION
#endif

#define QTEST_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    QApplication app(argc, argv); \
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    QTEST_DISABLE_KEYPAD_NAVIGATION \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#elif defined(QT_GUI_LIB)

#include <QtTest/qtest_gui.h>

#define QTEST_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    QGuiApplication app(argc, argv); \
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#else

#define QTEST_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    QCoreApplication app(argc, argv); \
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#endif // QT_GUI_LIB

#define QTEST_GUILESS_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    QCoreApplication app(argc, argv); \
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#endif
