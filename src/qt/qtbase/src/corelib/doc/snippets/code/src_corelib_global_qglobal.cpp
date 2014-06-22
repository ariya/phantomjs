/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
//! [0]


//! [1]
class MyClass
{
public:
    enum Option {
        NoOptions = 0x0,
        ShowTabs = 0x1,
        ShowAll = 0x2,
        SqueezeBlank = 0x4
    };
    Q_DECLARE_FLAGS(Options, Option)
    ...
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MyClass::Options)
//! [1]

//! [meta-object flags]
Q_FLAGS(Options)
//! [meta-object flags]

//! [2]
typedef QFlags<Enum> Flags;
//! [2]


//! [3]
int myValue = 10;
int minValue = 2;
int maxValue = 6;

int boundedValue = qBound(minValue, myValue, maxValue);
// boundedValue == 6
//! [3]


//! [4]
if (!driver()->isOpen() || driver()->isOpenError()) {
    qWarning("QSqlQuery::exec: database not open");
    return false;
}
//! [4]


//! [5]
qint64 value = Q_INT64_C(932838457459459);
//! [5]


//! [6]
quint64 value = Q_UINT64_C(932838457459459);
//! [6]


//! [7]
void myMsgHandler(QtMsgType, const char *);
//! [7]


//! [8]
qint64 value = Q_INT64_C(932838457459459);
//! [8]


//! [9]
quint64 value = Q_UINT64_C(932838457459459);
//! [9]


//! [10]
int absoluteValue;
int myValue = -4;

absoluteValue = qAbs(myValue);
// absoluteValue == 4
//! [10]


//! [11]
qreal valueA = 2.3;
qreal valueB = 2.7;

int roundedValueA = qRound(valueA);
// roundedValueA = 2
int roundedValueB = qRound(valueB);
// roundedValueB = 3
//! [11]


//! [12]
qreal valueA = 42949672960.3;
qreal valueB = 42949672960.7;

int roundedValueA = qRound(valueA);
// roundedValueA = 42949672960
int roundedValueB = qRound(valueB);
// roundedValueB = 42949672961
//! [12]


//! [13]
int myValue = 6;
int yourValue = 4;

int minValue = qMin(myValue, yourValue);
// minValue == yourValue
//! [13]


//! [14]
int myValue = 6;
int yourValue = 4;

int maxValue = qMax(myValue, yourValue);
// maxValue == myValue
//! [14]


//! [15]
int myValue = 10;
int minValue = 2;
int maxValue = 6;

int boundedValue = qBound(minValue, myValue, maxValue);
// boundedValue == 6
//! [15]


//! [16]
#if QT_VERSION >= 0x040100
    QIcon icon = style()->standardIcon(QStyle::SP_TrashIcon);
#else
    QPixmap pixmap = style()->standardPixmap(QStyle::SP_TrashIcon);
    QIcon icon(pixmap);
#endif
//! [16]


//! [17]
// File: div.cpp

#include <QtGlobal>

int divide(int a, int b)
{
    Q_ASSERT(b != 0);
    return a / b;
}
//! [17]


//! [18]
ASSERT: "b != 0" in file div.cpp, line 7
//! [18]


//! [19]
// File: div.cpp

#include <QtGlobal>

int divide(int a, int b)
{
    Q_ASSERT_X(b != 0, "divide", "division by zero");
    return a / b;
}
//! [19]


//! [20]
ASSERT failure in divide: "division by zero", file div.cpp, line 7
//! [20]


//! [21]
int *a;

Q_CHECK_PTR(a = new int[80]);   // WRONG!

a = new (nothrow) int[80];      // Right
Q_CHECK_PTR(a);
//! [21]


//! [22]
template<typename TInputType>
const TInputType &myMin(const TInputType &value1, const TInputType &value2)
{
    qDebug() << Q_FUNC_INFO << "was called with value1:" << value1 << "value2:" << value2;

    if(value1 < value2)
        return value1;
    else
        return value2;
}
//! [22]


//! [23]
#include <qapplication.h>
#include <stdio.h>
#include <stdlib.h>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}

int main(int argc, char **argv)
{
    qInstallMessageHandler(myMessageOutput);
    QApplication app(argc, argv);
    ...
    return app.exec();
}
//! [23]


//! [24]
qDebug("Items in list: %d", myList.size());
//! [24]


//! [25]
qDebug() << "Brush:" << myQBrush << "Other value:" << i;
//! [25]


//! [26]
void f(int c)
{
    if (c > 200)
        qWarning("f: bad argument, c == %d", c);
}
//! [26]


//! [27]
qWarning() << "Brush:" << myQBrush << "Other value:"
<< i;
//! [27]


//! [28]
void load(const QString &fileName)
{
    QFile file(fileName);
    if (!file.exists())
        qCritical("File '%s' does not exist!", qPrintable(fileName));
}
//! [28]


//! [29]
qCritical() << "Brush:" << myQBrush << "Other
value:" << i;
//! [29]


//! [30]
int divide(int a, int b)
{
    if (b == 0)                                // program error
        qFatal("divide: cannot divide by zero");
    return a / b;
}
//! [30]


//! [31]
forever {
    ...
}
//! [31]


//! [32]
CONFIG += no_keywords
//! [32]


//! [33]
CONFIG += no_keywords
//! [33]


//! [34]
QString FriendlyConversation::greeting(int type)
{
static const char *greeting_strings[] = {
    QT_TR_NOOP("Hello"),
    QT_TR_NOOP("Goodbye")
};
return tr(greeting_strings[type]);
}
//! [34]


//! [35]
static const char *greeting_strings[] = {
    QT_TRANSLATE_NOOP("FriendlyConversation", "Hello"),
    QT_TRANSLATE_NOOP("FriendlyConversation", "Goodbye")
};

QString FriendlyConversation::greeting(int type)
{
    return tr(greeting_strings[type]);
}

QString global_greeting(int type)
{
    return qApp->translate("FriendlyConversation",
           greeting_strings[type]);
}
//! [35]


//! [36]

static { const char *source; const char *comment; } greeting_strings[] =
{
    QT_TRANSLATE_NOOP3("FriendlyConversation", "Hello",
                       "A really friendly hello"),
    QT_TRANSLATE_NOOP3("FriendlyConversation", "Goodbye",
                       "A really friendly goodbye")
};

QString FriendlyConversation::greeting(int type)
{
    return tr(greeting_strings[type].source,
              greeting_strings[type].comment);
}

QString global_greeting(int type)
{
    return qApp->translate("FriendlyConversation",
           greeting_strings[type].source,
           greeting_strings[type].comment);
}
//! [36]


//! [qttrid]
    //% "%n fooish bar(s) found.\n"
    //% "Do you want to continue?"
    QString text = qtTrId("qtn_foo_bar", n);
//! [qttrid]


//! [qttrid_noop]
static const char * const ids[] = {
    //% "This is the first text."
    QT_TRID_NOOP("qtn_1st_text"),
    //% "This is the second text."
    QT_TRID_NOOP("qtn_2nd_text"),
    0
};

void TheClass::addLabels()
{
    for (int i = 0; ids[i]; ++i)
        new QLabel(qtTrId(ids[i]), this);
}
//! [qttrid_noop]


//! [37]
qWarning("%s: %s", qPrintable(key), qPrintable(value));
//! [37]


//! [38]
struct Point2D
{
    int x;
    int y;
};

Q_DECLARE_TYPEINFO(Point2D, Q_PRIMITIVE_TYPE);
//! [38]


//! [39]
class Point2D
{
public:
    Point2D() { data = new int[2]; }
    Point2D(const Point2D &other) { ... }
    ~Point2D() { delete[] data; }

    Point2D &operator=(const Point2D &other) { ... }

    int x() const { return data[0]; }
    int y() const { return data[1]; }

private:
    int *data;
};

Q_DECLARE_TYPEINFO(Point2D, Q_MOVABLE_TYPE);
//! [39]


//! [40]
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
...
#endif

or

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
...
#endif

//! [40]


//! [41]

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
...
#endif

//! [41]


//! [42]
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
...
#endif

//! [42]

//! [begin namespace macro]
namespace QT_NAMESPACE {
//! [begin namespace macro]

//! [end namespace macro]
}
//! [end namespace macro]

//! [43]
class MyClass : public QObject
{

  private:
    Q_DISABLE_COPY(MyClass)
};

//! [43]

//! [44]
class MyClass : public QObject
{

  private:
     MyClass(const MyClass &);
     MyClass &operator=(const MyClass &);
};
//! [44]

//! [45]
  QWidget w = QWidget();
//! [45]

//! [46]
        // Instead of comparing with 0.0
                qFuzzyCompare(0.0,1.0e-200); // This will return false
        // Compare adding 1 to both values will fix the problem
                qFuzzyCompare(1 + 0.0, 1 + 1.0e-200); // This will return true
//! [46]

//! [47]
CApaApplication *myApplicationFactory();
//! [47]


//! [49]
void myMessageHandler(QtMsgType, const QMessageLogContext &, const QString &);
//! [49]

//! [50]
class B {...};
class C {...};
class D {...};
struct A : public B {
    C c;
    D d;
};
//! [50]

//! [51]
template<> class QTypeInfo<A> : public QTypeInfoMerger<A, B, C, D> {};
//! [51]

//! [qlikely]
    // the condition inside the "if" will be successful most of the times
    for (int i = 1; i <= 365; i++) {
        if (Q_LIKELY(isWorkingDay(i))) {
            ...
        }
        ...
    }
//! [qlikely]

//! [qunlikely]
bool readConfiguration(const QFile &file)
{
    // We expect to be asked to read an existing file
    if (Q_UNLIKELY(!file.exists())) {
        qWarning() << "File not found";
        return false;
    }

    ...
    return true;
}
//! [qunlikely]

//! [qunreachable-enum]
   enum Shapes {
       Rectangle,
       Triangle,
       Circle,
       NumShapes
   };
//! [qunreachable-enum]

//! [qunreachable-switch]
   switch (shape) {
       case Rectangle:
           return rectangle();
       case Triangle:
           return triangle();
       case Circle:
           return circle();
       case NumShapes:
           Q_UNREACHABLE();
           break;
   }
//! [qunreachable-switch]

//! [qt-version-check]
#include <QtGlobal>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets>
#else
#include <QtGui>
#endif
//! [qt-version-check]
