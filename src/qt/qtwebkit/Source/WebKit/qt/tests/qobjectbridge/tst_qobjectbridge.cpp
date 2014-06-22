/*
    Copyright (C) 2008,2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QtTest/QtTest>

#include <qbrush.h>
#include <qwebelement.h>
#include <qwebframe.h>
#include <qwebpage.h>
#include <qwebview.h>

struct CustomType {
    QString string;
};
Q_DECLARE_METATYPE(CustomType)

Q_DECLARE_METATYPE(QBrush*)
Q_DECLARE_METATYPE(QObjectList)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QVariantList)
Q_DECLARE_METATYPE(QVariantMap)

class MyQObject : public QObject {
    Q_OBJECT

    Q_PROPERTY(int intProperty READ intProperty WRITE setIntProperty)
    Q_PROPERTY(QVariant variantProperty READ variantProperty WRITE setVariantProperty)
    Q_PROPERTY(QVariantList variantListProperty READ variantListProperty WRITE setVariantListProperty)
    Q_PROPERTY(QVariantMap variantMapProperty READ variantMapProperty WRITE setVariantMapProperty)
    Q_PROPERTY(QString stringProperty READ stringProperty WRITE setStringProperty)
    Q_PROPERTY(QStringList stringListProperty READ stringListProperty WRITE setStringListProperty)
    Q_PROPERTY(QByteArray byteArrayProperty READ byteArrayProperty WRITE setByteArrayProperty)
    Q_PROPERTY(QBrush brushProperty READ brushProperty WRITE setBrushProperty)
    Q_PROPERTY(double hiddenProperty READ hiddenProperty WRITE setHiddenProperty SCRIPTABLE false)
    Q_PROPERTY(int writeOnlyProperty WRITE setWriteOnlyProperty)
    Q_PROPERTY(int readOnlyProperty READ readOnlyProperty)
    Q_PROPERTY(QKeySequence shortcut READ shortcut WRITE setShortcut)
    Q_PROPERTY(CustomType propWithCustomType READ propWithCustomType WRITE setPropWithCustomType)
    Q_PROPERTY(QWebElement webElementProperty READ webElementProperty WRITE setWebElementProperty)
    Q_PROPERTY(QObject* objectStarProperty READ objectStarProperty WRITE setObjectStarProperty)
    Q_ENUMS(Policy Strategy)
    Q_FLAGS(Ability)

public:
    enum Policy {
        FooPolicy = 0,
        BarPolicy,
        BazPolicy
    };

    enum Strategy {
        FooStrategy = 10,
        BarStrategy,
        BazStrategy
    };

    enum AbilityFlag {
        NoAbility  = 0x000,
        FooAbility = 0x001,
        BarAbility = 0x080,
        BazAbility = 0x200,
        AllAbility = FooAbility | BarAbility | BazAbility
    };

    Q_DECLARE_FLAGS(Ability, AbilityFlag)

    MyQObject(QObject* parent = 0)
        : QObject(parent),
            m_intValue(123),
            m_variantValue(QLatin1String("foo")),
            m_variantListValue(QVariantList() << QVariant(123) << QVariant(QLatin1String("foo"))),
            m_stringValue(QLatin1String("bar")),
            m_stringListValue(QStringList() << QLatin1String("zig") << QLatin1String("zag")),
            m_brushValue(QColor(10, 20, 30, 40)),
            m_hiddenValue(456.0),
            m_writeOnlyValue(789),
            m_readOnlyValue(987),
            m_objectStar(0),
            m_qtFunctionInvoked(-1)
    {
        m_variantMapValue.insert("a", QVariant(123));
        m_variantMapValue.insert("b", QVariant(QLatin1String("foo")));
        m_variantMapValue.insert("c", QVariant::fromValue<QObject*>(this));
    }

    ~MyQObject() { }

    int intProperty() const
    {
        return m_intValue;
    }
    void setIntProperty(int value)
    {
        m_intValue = value;
    }

    QVariant variantProperty() const
    {
        return m_variantValue;
    }
    void setVariantProperty(const QVariant& value)
    {
        m_variantValue = value;
    }

    QVariantList variantListProperty() const
    {
        return m_variantListValue;
    }
    void setVariantListProperty(const QVariantList& value)
    {
        m_variantListValue = value;
    }

    QVariantMap variantMapProperty() const
    {
        return m_variantMapValue;
    }
    void setVariantMapProperty(const QVariantMap& value)
    {
        m_variantMapValue = value;
    }

    QString stringProperty() const
    {
        return m_stringValue;
    }
    void setStringProperty(const QString& value)
    {
        m_stringValue = value;
    }

    QStringList stringListProperty() const
    {
        return m_stringListValue;
    }
    void setStringListProperty(const QStringList& value)
    {
        m_stringListValue = value;
    }

    QByteArray byteArrayProperty() const
    {
        return m_byteArrayValue;
    }
    void setByteArrayProperty(const QByteArray& value)
    {
        m_byteArrayValue = value;
    }

    QBrush brushProperty() const
    {
        return m_brushValue;
    }
    void setBrushProperty(const QBrush& value)
    {
        m_brushValue = value;
    }

    double hiddenProperty() const
    {
        return m_hiddenValue;
    }
    void setHiddenProperty(double value)
    {
        m_hiddenValue = value;
    }

    int writeOnlyProperty() const
    {
        return m_writeOnlyValue;
    }
    void setWriteOnlyProperty(int value)
    {
        m_writeOnlyValue = value;
    }

    int readOnlyProperty() const
    {
        return m_readOnlyValue;
    }

    QKeySequence shortcut() const
    {
        return m_shortcut;
    }
    void setShortcut(const QKeySequence& seq)
    {
        m_shortcut = seq;
    }

    QWebElement webElementProperty() const
    {
        return m_webElement;
    }
    void setWebElementProperty(const QWebElement& element)
    {
        m_webElement = element;
    }

    CustomType propWithCustomType() const
    {
        return m_customType;
    }
    void setPropWithCustomType(const CustomType& c)
    {
        m_customType = c;
    }

    QObject* objectStarProperty() const
    {
        return m_objectStar;
    }
    void setObjectStarProperty(QObject* object)
    {
        m_objectStar = object;
    }


    int qtFunctionInvoked() const
    {
        return m_qtFunctionInvoked;
    }

    QVariantList qtFunctionActuals() const
    {
        return m_actuals;
    }

    void resetQtFunctionInvoked()
    {
        m_qtFunctionInvoked = -1;
        m_actuals.clear();
    }

    Q_INVOKABLE void myInvokable()
    {
        m_qtFunctionInvoked = 0;
    }
    Q_INVOKABLE void myInvokableWithIntArg(int arg)
    {
        m_qtFunctionInvoked = 1;
        m_actuals << arg;
    }
    Q_INVOKABLE void myInvokableWithLonglongArg(qlonglong arg)
    {
        m_qtFunctionInvoked = 2;
        m_actuals << arg;
    }
    Q_INVOKABLE void myInvokableWithFloatArg(float arg)
    {
        m_qtFunctionInvoked = 3;
        m_actuals << arg;
    }
    Q_INVOKABLE void myInvokableWithDoubleArg(double arg)
    {
        m_qtFunctionInvoked = 4;
        m_actuals << arg;
    }
    Q_INVOKABLE void myInvokableWithStringArg(const QString& arg)
    {
        m_qtFunctionInvoked = 5;
        m_actuals << arg;
    }
    Q_INVOKABLE void myInvokableWithIntArgs(int arg1, int arg2)
    {
        m_qtFunctionInvoked = 6;
        m_actuals << arg1 << arg2;
    }
    Q_INVOKABLE int myInvokableReturningInt()
    {
        m_qtFunctionInvoked = 7;
        return 123;
    }
    Q_INVOKABLE qlonglong myInvokableReturningLongLong()
    {
        m_qtFunctionInvoked = 39;
        return 456;
    }
    Q_INVOKABLE QString myInvokableReturningString()
    {
        m_qtFunctionInvoked = 8;
        return QLatin1String("ciao");
    }
    Q_INVOKABLE void myInvokableWithIntArg(int arg1, int arg2) // Overload.
    {
        m_qtFunctionInvoked = 9;
        m_actuals << arg1 << arg2;
    }
    Q_INVOKABLE void myInvokableWithEnumArg(Policy policy)
    {
        m_qtFunctionInvoked = 10;
        m_actuals << policy;
    }
    Q_INVOKABLE void myInvokableWithQualifiedEnumArg(MyQObject::Policy policy)
    {
        m_qtFunctionInvoked = 36;
        m_actuals << policy;
    }
    Q_INVOKABLE Policy myInvokableReturningEnum()
    {
        m_qtFunctionInvoked = 37;
        return BazPolicy;
    }
    Q_INVOKABLE MyQObject::Policy myInvokableReturningQualifiedEnum()
    {
        m_qtFunctionInvoked = 38;
        return BazPolicy;
    }
    Q_INVOKABLE QVector<int> myInvokableReturningVectorOfInt()
    {
        m_qtFunctionInvoked = 11;
        return QVector<int>();
    }
    Q_INVOKABLE void myInvokableWithVectorOfIntArg(const QVector<int>&)
    {
        m_qtFunctionInvoked = 12;
    }
    Q_INVOKABLE QObject* myInvokableReturningQObjectStar()
    {
        m_qtFunctionInvoked = 13;
        return this;
    }
    Q_INVOKABLE QObjectList myInvokableWithQObjectListArg(const QObjectList& lst)
    {
        m_qtFunctionInvoked = 14;
        m_actuals << QVariant::fromValue(lst);
        return lst;
    }
    Q_INVOKABLE QVariant myInvokableWithVariantArg(const QVariant& v)
    {
        m_qtFunctionInvoked = 15;
        m_actuals << v;
        return v;
    }
    Q_INVOKABLE QVariantMap myInvokableWithVariantMapArg(const QVariantMap& vm)
    {
        m_qtFunctionInvoked = 16;
        m_actuals << vm;
        return vm;
    }
    Q_INVOKABLE QList<int> myInvokableWithListOfIntArg(const QList<int>& lst)
    {
        m_qtFunctionInvoked = 17;
        m_actuals << QVariant::fromValue(lst);
        return lst;
    }
    Q_INVOKABLE QObject* myInvokableWithQObjectStarArg(QObject* obj)
    {
        m_qtFunctionInvoked = 18;
        m_actuals << QVariant::fromValue(obj);
        return obj;
    }
    Q_INVOKABLE QBrush myInvokableWithQBrushArg(const QBrush& brush)
    {
        m_qtFunctionInvoked = 19;
        m_actuals << QVariant::fromValue(brush);
        return brush;
    }
    Q_INVOKABLE void myInvokableWithBrushStyleArg(Qt::BrushStyle style)
    {
        m_qtFunctionInvoked = 43;
        // Qt::BrushStyle isn't registered and this shouldn't be reached.
        QVERIFY(false);
    }
    Q_INVOKABLE void myInvokableWithVoidStarArg(void* arg)
    {
        m_qtFunctionInvoked = 44;
        m_actuals << QVariant::fromValue(arg);
    }
    Q_INVOKABLE void myInvokableWithAmbiguousArg(int arg)
    {
        m_qtFunctionInvoked = 45;
        m_actuals << QVariant::fromValue(arg);
    }
    Q_INVOKABLE void myInvokableWithAmbiguousArg(uint arg)
    {
        m_qtFunctionInvoked = 46;
        m_actuals << QVariant::fromValue(arg);
    }
    Q_INVOKABLE void myInvokableWithDefaultArgs(int arg1, const QString& arg2 = QString())
    {
        m_qtFunctionInvoked = 47;
        m_actuals << QVariant::fromValue(arg1) << qVariantFromValue(arg2);
    }
    Q_INVOKABLE QObject& myInvokableReturningRef()
    {
        m_qtFunctionInvoked = 48;
        return *this;
    }
    Q_INVOKABLE const QObject& myInvokableReturningConstRef() const
    {
        const_cast<MyQObject*>(this)->m_qtFunctionInvoked = 49;
        return *this;
    }
    Q_INVOKABLE void myInvokableWithPointArg(const QPoint &arg) {
        const_cast<MyQObject*>(this)->m_qtFunctionInvoked = 50;
        m_actuals << QVariant::fromValue(arg);
    }
    Q_INVOKABLE void myInvokableWithPointArg(const QPointF &arg) {
        const_cast<MyQObject*>(this)->m_qtFunctionInvoked = 51;
        m_actuals << QVariant::fromValue(arg);
    }
    Q_INVOKABLE void myInvokableWithBoolArg(bool arg) {
        m_qtFunctionInvoked = 52;
        m_actuals << arg;
    }

    void emitMySignal()
    {
        emit mySignal();
    }
    void emitMySignalWithIntArg(int arg)
    {
        emit mySignalWithIntArg(arg);
    }
    void emitMySignal2(bool arg)
    {
        emit mySignal2(arg);
    }
    void emitMySignal2()
    {
        emit mySignal2();
    }
    void emitMySignalWithDateTimeArg(QDateTime dt)
    {
        emit mySignalWithDateTimeArg(dt);
    }

public Q_SLOTS:
    void mySlot()
    {
        m_qtFunctionInvoked = 20;
    }

    void mySlotWithIntArg(int arg)
    {
        m_qtFunctionInvoked = 21;
        m_actuals << arg;
    }

    void mySlotWithDoubleArg(double arg)
    {
        m_qtFunctionInvoked = 22;
        m_actuals << arg;
    }

    void mySlotWithStringArg(const QString &arg)
    {
        m_qtFunctionInvoked = 23;
        m_actuals << arg;
    }

    void myOverloadedSlot()
    {
        m_qtFunctionInvoked = 24;
    }

    void myOverloadedSlot(QObject* arg)
    {
        m_qtFunctionInvoked = 41;
        m_actuals << QVariant::fromValue(arg);
    }

    void myOverloadedSlot(bool arg)
    {
        m_qtFunctionInvoked = 25;
        m_actuals << arg;
    }

    void myOverloadedSlot(const QStringList &arg)
    {
        m_qtFunctionInvoked = 42;
        m_actuals << arg;
    }

    void myOverloadedSlot(double arg)
    {
        m_qtFunctionInvoked = 26;
        m_actuals << arg;
    }

    void myOverloadedSlot(float arg)
    {
        m_qtFunctionInvoked = 27;
        m_actuals << arg;
    }

    void myOverloadedSlot(int arg)
    {
        m_qtFunctionInvoked = 28;
        m_actuals << arg;
    }

    void myOverloadedSlot(const QString &arg)
    {
        m_qtFunctionInvoked = 29;
        m_actuals << arg;
    }

    void myOverloadedSlot(const QColor &arg)
    {
        m_qtFunctionInvoked = 30;
        m_actuals << arg;
    }

    void myOverloadedSlot(const QBrush &arg)
    {
        m_qtFunctionInvoked = 31;
        m_actuals << arg;
    }

    void myOverloadedSlot(const QDateTime &arg)
    {
        m_qtFunctionInvoked = 32;
        m_actuals << arg;
    }

    void myOverloadedSlot(const QDate &arg)
    {
        m_qtFunctionInvoked = 33;
        m_actuals << arg;
    }

    void myOverloadedSlot(const QVariant &arg)
    {
        m_qtFunctionInvoked = 35;
        m_actuals << arg;
    }

    void myOverloadedSlot(const QWebElement &arg)
    {
        m_qtFunctionInvoked = 36;
        m_actuals << QVariant::fromValue<QWebElement>(arg);
    }

protected Q_SLOTS:
    void myProtectedSlot()
    {
        m_qtFunctionInvoked = 36;
    }

private Q_SLOTS:
    void myPrivateSlot() { }

Q_SIGNALS:
    void mySignal();
    void mySignalWithIntArg(int);
    void mySignalWithDoubleArg(double);
    void mySignal2(bool arg = false);
    void mySignalWithDateTimeArg(QDateTime);

private:
    int m_intValue;
    QVariant m_variantValue;
    QVariantList m_variantListValue;
    QVariantMap m_variantMapValue;
    QString m_stringValue;
    QStringList m_stringListValue;
    QByteArray m_byteArrayValue;
    QBrush m_brushValue;
    double m_hiddenValue;
    int m_writeOnlyValue;
    int m_readOnlyValue;
    QKeySequence m_shortcut;
    QWebElement m_webElement;
    CustomType m_customType;
    QObject* m_objectStar;
    int m_qtFunctionInvoked;
    QVariantList m_actuals;
};

class MyWebElementSlotOnlyObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString tagName READ tagName)
public Q_SLOTS:
    void doSomethingWithWebElement(const QWebElement& element)
    {
        m_tagName = element.tagName();
    }

public:
    QString tagName() const
    {
        return m_tagName;
    }
private:
    QString m_tagName;
};

class MyOtherQObject : public MyQObject {
public:
    MyOtherQObject(QObject* parent = 0)
        : MyQObject(parent) { }
};

class tst_QObjectBridge : public QObject {
    Q_OBJECT

public:
    tst_QObjectBridge();

public Q_SLOTS:
    void init();
    void cleanup();

private Q_SLOTS:
    void getSetStaticProperty();
    void getSetDynamicProperty();
    void getSetChildren();
    void callQtInvokable();
    void connectAndDisconnect();
    void overrideInvokable();
    void overloadedSlots();
    void webElementSlotOnly();
    void enumerate_data();
    void enumerate();
    void objectDeleted();
    void typeConversion();
    void arrayObjectEnumerable();
    void domCycles();
    void jsByteArray();
    void ownership();
    void nullValue();
    void qObjectWrapperWithSameIdentity();
    void introspectQtMethods_data();
    void introspectQtMethods();
    void scriptablePlugin();

private:
    QString evalJS(const QString& s)
    {
        QVariant ret = evalJSV(s);
        if (!ret.isValid())
            return "undefined";
        return ret.toString();
    }

    QVariant evalJSV(const QString& s)
    {
        return m_page->mainFrame()->evaluateJavaScript(s);
    }

    QString evalJS(const QString& s, QString& type)
    {
        return evalJSV(s, type).toString();
    }

    QVariant evalJSV(const QString& s, QString& type)
    {
        // As a special measure, if we get an exception we set the type to 'error'
        // (in ecma, an Error object has typeof object, but qtscript has a convenience function)
        // Similarly, an array is an object, but we'd prefer to have a type of 'array'
        QString escaped = s;
        escaped.replace('\'', "\\'"); // Don't preescape your single quotes!
        QString code("var retvalue; "
                     "var typevalue; "
                     "try { "
                     "    retvalue = eval('%1'); "
                     "    typevalue = typeof retvalue; "
                     "    if (retvalue instanceof Array) "
                     "        typevalue = 'array'; "
                     "} catch(e) { "
                     "    retvalue = e.name + ': ' + e.message; "
                     "    typevalue = 'error'; "
                     "}");
        evalJS(code.arg(escaped));

        QVariant ret = evalJSV("retvalue");
        if (ret.isValid())
            type = evalJS("typevalue");
        else {
            ret = QString("undefined");
            type = sUndefined;
        }
        evalJS("delete retvalue; delete typevalue");
        return ret;
    }

    const QString sTrue;
    const QString sFalse;
    const QString sUndefined;
    const QString sArray;
    const QString sFunction;
    const QString sError;
    const QString sString;
    const QString sObject;
    const QString sNumber;

private:
    QWebView* m_view;
    QWebPage* m_page;
    MyQObject* m_myObject;
};

tst_QObjectBridge::tst_QObjectBridge()
    : sTrue("true")
    , sFalse("false")
    , sUndefined("undefined")
    , sArray("array")
    , sFunction("function")
    , sError("error")
    , sString("string")
    , sObject("object")
    , sNumber("number")
{
}

void tst_QObjectBridge::init()
{
    m_view = new QWebView();
    m_page = m_view->page();
    m_myObject = new MyQObject();
    m_page->mainFrame()->addToJavaScriptWindowObject("myObject", m_myObject);
}

void tst_QObjectBridge::cleanup()
{
    delete m_view;
    delete m_myObject;
}

void tst_QObjectBridge::getSetStaticProperty()
{
    m_page->mainFrame()->setHtml("<html><head><body></body></html>");
    QCOMPARE(evalJS("typeof myObject.noSuchProperty"), sUndefined);

    // initial value (set in MyQObject constructor)
    {
        QString type;
        QVariant ret = evalJSV("myObject.intProperty", type);
        QCOMPARE(type, sNumber);
        QCOMPARE(ret.type(), QVariant::Double);
        QCOMPARE(ret.toInt(), 123);
    }
    QCOMPARE(evalJS("myObject.intProperty === 123.0"), sTrue);

    {
        QString type;
        QVariant ret = evalJSV("myObject.variantProperty", type);
        QCOMPARE(type, sString);
        QCOMPARE(ret.type(), QVariant::String);
        QCOMPARE(ret.toString(), QLatin1String("foo"));
    }
    QCOMPARE(evalJS("myObject.variantProperty == 'foo'"), sTrue);

    {
        QString type;
        QVariant ret = evalJSV("myObject.stringProperty", type);
        QCOMPARE(type, sString);
        QCOMPARE(ret.type(), QVariant::String);
        QCOMPARE(ret.toString(), QLatin1String("bar"));
    }
    QCOMPARE(evalJS("myObject.stringProperty === 'bar'"), sTrue);

    {
        QString type;
        QVariant ret = evalJSV("myObject.variantListProperty", type);
        QCOMPARE(type, sArray);
        QCOMPARE(ret.type(), QVariant::List);
        QVariantList vl = ret.value<QVariantList>();
        QCOMPARE(vl.size(), 2);
        QCOMPARE(vl.at(0).toInt(), 123);
        QCOMPARE(vl.at(1).toString(), QLatin1String("foo"));
    }
    QCOMPARE(evalJS("myObject.variantListProperty.length === 2"), sTrue);
    QCOMPARE(evalJS("myObject.variantListProperty[0] === 123"), sTrue);
    QCOMPARE(evalJS("myObject.variantListProperty[1] === 'foo'"), sTrue);

    {
        QString type;
        QVariant ret = evalJSV("myObject.variantMapProperty", type);
        QCOMPARE(type, sObject);
        QCOMPARE(ret.type(), QVariant::Map);
        QVariantMap vm = ret.value<QVariantMap>();
        QCOMPARE(vm.size(), 3);
        QCOMPARE(vm.value("a").toInt(), 123);
        QCOMPARE(vm.value("b").toString(), QLatin1String("foo"));
        QCOMPARE(vm.value("c").value<QObject*>(), static_cast<QObject*>(m_myObject));
    }
    QCOMPARE(evalJS("myObject.variantMapProperty.a === 123"), sTrue);
    QCOMPARE(evalJS("myObject.variantMapProperty.b === 'foo'"), sTrue);
    QCOMPARE(evalJS("myObject.variantMapProperty.c.variantMapProperty.b === 'foo'"), sTrue);

    {
        QString type;
        QVariant ret = evalJSV("myObject.stringListProperty", type);
        QCOMPARE(type, sArray);
        QCOMPARE(ret.type(), QVariant::List);
        QVariantList vl = ret.value<QVariantList>();
        QCOMPARE(vl.size(), 2);
        QCOMPARE(vl.at(0).toString(), QLatin1String("zig"));
        QCOMPARE(vl.at(1).toString(), QLatin1String("zag"));
    }
    QCOMPARE(evalJS("myObject.stringListProperty.length === 2"), sTrue);
    QCOMPARE(evalJS("typeof myObject.stringListProperty[0]"), sString);
    QCOMPARE(evalJS("myObject.stringListProperty[0]"), QLatin1String("zig"));
    QCOMPARE(evalJS("typeof myObject.stringListProperty[1]"), sString);
    QCOMPARE(evalJS("myObject.stringListProperty[1]"), QLatin1String("zag"));

    // property change in C++ should be reflected in script
    m_myObject->setIntProperty(456);
    QCOMPARE(evalJS("myObject.intProperty == 456"), sTrue);
    m_myObject->setIntProperty(789);
    QCOMPARE(evalJS("myObject.intProperty == 789"), sTrue);

    m_myObject->setVariantProperty(QLatin1String("bar"));
    QCOMPARE(evalJS("myObject.variantProperty === 'bar'"), sTrue);
    m_myObject->setVariantProperty(42);
    QCOMPARE(evalJS("myObject.variantProperty === 42"), sTrue);
    m_myObject->setVariantProperty(QVariant::fromValue(QBrush()));

    m_myObject->setStringProperty(QLatin1String("baz"));
    QCOMPARE(evalJS("myObject.stringProperty === 'baz'"), sTrue);
    m_myObject->setStringProperty(QLatin1String("zab"));
    QCOMPARE(evalJS("myObject.stringProperty === 'zab'"), sTrue);

    // property change in script should be reflected in C++
    QCOMPARE(evalJS("myObject.intProperty = 123"), QLatin1String("123"));
    QCOMPARE(evalJS("myObject.intProperty == 123"), sTrue);
    QCOMPARE(m_myObject->intProperty(), 123);
    QCOMPARE(evalJS("myObject.intProperty = 'ciao!';"
                    "myObject.intProperty == 0"), sTrue);
    QCOMPARE(m_myObject->intProperty(), 0);
    QCOMPARE(evalJS("myObject.intProperty = '123';"
                    "myObject.intProperty == 123"), sTrue);
    QCOMPARE(m_myObject->intProperty(), 123);

    QCOMPARE(evalJS("myObject.stringProperty = 'ciao'"), QLatin1String("ciao"));
    QCOMPARE(evalJS("myObject.stringProperty"), QLatin1String("ciao"));
    QCOMPARE(m_myObject->stringProperty(), QLatin1String("ciao"));
    QCOMPARE(evalJS("myObject.stringProperty = 123;"
                    "myObject.stringProperty"), QLatin1String("123"));
    QCOMPARE(m_myObject->stringProperty(), QLatin1String("123"));
    QCOMPARE(evalJS("myObject.stringProperty = null"), QString());
    QCOMPARE(evalJS("myObject.stringProperty"), QString());
    QCOMPARE(m_myObject->stringProperty(), QString());
    QCOMPARE(evalJS("myObject.stringProperty = undefined"), sUndefined);
    QCOMPARE(evalJS("myObject.stringProperty"), QString());
    QCOMPARE(m_myObject->stringProperty(), QString());

    QCOMPARE(evalJS("myObject.variantProperty = new Number(1234);"
                    "myObject.variantProperty").toDouble(), 1234.0);
    QCOMPARE(m_myObject->variantProperty().toDouble(), 1234.0);

    QCOMPARE(evalJS("myObject.variantProperty = new Boolean(1234);"
                    "myObject.variantProperty"), sTrue);
    QCOMPARE(m_myObject->variantProperty().toBool(), true);

    QCOMPARE(evalJS("myObject.variantProperty = null;"
                    "myObject.variantProperty.valueOf()"), sUndefined);
    QCOMPARE(m_myObject->variantProperty(), QVariant());
    QCOMPARE(evalJS("myObject.variantProperty = undefined;"
                    "myObject.variantProperty.valueOf()"), sUndefined);
    QCOMPARE(m_myObject->variantProperty(), QVariant());

    QCOMPARE(evalJS("myObject.variantProperty = 'foo';"
                    "myObject.variantProperty.valueOf()"), QLatin1String("foo"));
    QCOMPARE(m_myObject->variantProperty(), QVariant(QLatin1String("foo")));
    QCOMPARE(evalJS("myObject.variantProperty = 42;"
                    "myObject.variantProperty").toDouble(), 42.0);
    QCOMPARE(m_myObject->variantProperty().toDouble(), 42.0);

    QCOMPARE(evalJS("myObject.variantListProperty = [1, 'two', true];"
                    "myObject.variantListProperty.length == 3"), sTrue);
    QCOMPARE(evalJS("myObject.variantListProperty[0] === 1"), sTrue);
    QCOMPARE(evalJS("myObject.variantListProperty[1]"), QLatin1String("two"));
    QCOMPARE(evalJS("myObject.variantListProperty[2] === true"), sTrue);

    QCOMPARE(evalJS("myObject.stringListProperty = [1, 'two', true];"
                    "myObject.stringListProperty.length == 3"), sTrue);
    QCOMPARE(evalJS("typeof myObject.stringListProperty[0]"), sString);
    QCOMPARE(evalJS("myObject.stringListProperty[0] == '1'"), sTrue);
    QCOMPARE(evalJS("typeof myObject.stringListProperty[1]"), sString);
    QCOMPARE(evalJS("myObject.stringListProperty[1]"), QLatin1String("two"));
    QCOMPARE(evalJS("typeof myObject.stringListProperty[2]"), sString);
    QCOMPARE(evalJS("myObject.stringListProperty[2]"), QLatin1String("true"));
    evalJS("myObject.webElementProperty=document.body;");
    QCOMPARE(evalJS("myObject.webElementProperty.tagName"), QLatin1String("BODY"));

    // try to delete
    QCOMPARE(evalJS("delete myObject.intProperty"), sFalse);
    QCOMPARE(evalJS("myObject.intProperty == 123"), sTrue);

    QCOMPARE(evalJS("delete myObject.variantProperty"), sFalse);
    QCOMPARE(evalJS("myObject.variantProperty").toDouble(), 42.0);

    // custom property
    QCOMPARE(evalJS("myObject.customProperty"), sUndefined);
    QCOMPARE(evalJS("myObject.customProperty = 123;"
                    "myObject.customProperty == 123"), sTrue);
    QVariant v = m_page->mainFrame()->evaluateJavaScript("myObject.customProperty");
    QCOMPARE(v.type(), QVariant::Double);
    QCOMPARE(v.toInt(), 123);

    // non-scriptable property
    QCOMPARE(m_myObject->hiddenProperty(), 456.0);
    QCOMPARE(evalJS("myObject.hiddenProperty"), sUndefined);
    QCOMPARE(evalJS("myObject.hiddenProperty = 123;"
                    "myObject.hiddenProperty == 123"), sTrue);
    QCOMPARE(m_myObject->hiddenProperty(), 456.0);

    // write-only property
    QCOMPARE(m_myObject->writeOnlyProperty(), 789);
    QCOMPARE(evalJS("typeof myObject.writeOnlyProperty"), sUndefined);
    QCOMPARE(evalJS("myObject.writeOnlyProperty = 123;"
                    "typeof myObject.writeOnlyProperty"), sUndefined);
    QCOMPARE(m_myObject->writeOnlyProperty(), 123);

    // read-only property
    QCOMPARE(m_myObject->readOnlyProperty(), 987);
    QCOMPARE(evalJS("myObject.readOnlyProperty == 987"), sTrue);
    QCOMPARE(evalJS("myObject.readOnlyProperty = 654;"
                    "myObject.readOnlyProperty == 987"), sTrue);
    QCOMPARE(m_myObject->readOnlyProperty(), 987);

    // QObject* property
    m_myObject->setObjectStarProperty(0);
    QCOMPARE(m_myObject->objectStarProperty(), (QObject*)0);
    QCOMPARE(evalJS("myObject.objectStarProperty == null"), sTrue);
    QCOMPARE(evalJS("typeof myObject.objectStarProperty"), sObject);
    QCOMPARE(evalJS("Boolean(myObject.objectStarProperty)"), sFalse);
    QCOMPARE(evalJS("String(myObject.objectStarProperty) == 'null'"), sTrue);
    QCOMPARE(evalJS("myObject.objectStarProperty.objectStarProperty"),
        sUndefined);
    m_myObject->setObjectStarProperty(this);
    QCOMPARE(evalJS("myObject.objectStarProperty != null"), sTrue);
    QCOMPARE(evalJS("typeof myObject.objectStarProperty"), sObject);
    QCOMPARE(evalJS("Boolean(myObject.objectStarProperty)"), sTrue);
    QCOMPARE(evalJS("String(myObject.objectStarProperty) != 'null'"), sTrue);
}

void tst_QObjectBridge::getSetDynamicProperty()
{
    // initially the object does not have the property
    // In WebKit, RuntimeObjects do not inherit Object, so don't have hasOwnProperty

    // QCOMPARE(evalJS("myObject.hasOwnProperty('dynamicProperty')"), sFalse);
    QCOMPARE(evalJS("typeof myObject.dynamicProperty"), sUndefined);

    // add a dynamic property in C++
    QCOMPARE(m_myObject->setProperty("dynamicProperty", 123), false);
    // QCOMPARE(evalJS("myObject.hasOwnProperty('dynamicProperty')"), sTrue);
    QCOMPARE(evalJS("typeof myObject.dynamicProperty != 'undefined'"), sTrue);
    QCOMPARE(evalJS("myObject.dynamicProperty == 123"), sTrue);

    // property change in script should be reflected in C++
    QCOMPARE(evalJS("myObject.dynamicProperty = 'foo';"
                    "myObject.dynamicProperty"), QLatin1String("foo"));
    QCOMPARE(m_myObject->property("dynamicProperty").toString(), QLatin1String("foo"));

    // delete the property (XFAIL - can't delete properties)
    QEXPECT_FAIL("", "can't delete properties", Continue);
    QCOMPARE(evalJS("delete myObject.dynamicProperty"), sTrue);
    /*
    QCOMPARE(m_myObject->property("dynamicProperty").isValid(), false);
    QCOMPARE(evalJS("typeof myObject.dynamicProperty"), sUndefined);
    //    QCOMPARE(evalJS("myObject.hasOwnProperty('dynamicProperty')"), sFalse);
    QCOMPARE(evalJS("typeof myObject.dynamicProperty"), sUndefined);
    */
}

void tst_QObjectBridge::getSetChildren()
{
    // initially the object does not have the child
    // (again, no hasOwnProperty)

    // QCOMPARE(evalJS("myObject.hasOwnProperty('child')"), sFalse);
    QCOMPARE(evalJS("typeof myObject.child"), sUndefined);

    // add a child
    MyQObject* child = new MyQObject(m_myObject);
    child->setObjectName("child");
//  QCOMPARE(evalJS("myObject.hasOwnProperty('child')"), sTrue);
    QCOMPARE(evalJS("typeof myObject.child != 'undefined'"), sTrue);

    // add a grandchild
    MyQObject* grandChild = new MyQObject(child);
    grandChild->setObjectName("grandChild");
//  QCOMPARE(evalJS("myObject.child.hasOwnProperty('grandChild')"), sTrue);
    QCOMPARE(evalJS("typeof myObject.child.grandChild != 'undefined'"), sTrue);

    // delete grandchild
    delete grandChild;
//  QCOMPARE(evalJS("myObject.child.hasOwnProperty('grandChild')"), sFalse);
    QCOMPARE(evalJS("typeof myObject.child.grandChild == 'undefined'"), sTrue);

    // delete child
    delete child;
//  QCOMPARE(evalJS("myObject.hasOwnProperty('child')"), sFalse);
    QCOMPARE(evalJS("typeof myObject.child == 'undefined'"), sTrue);
}

Q_DECLARE_METATYPE(QVector<int>)
Q_DECLARE_METATYPE(QVector<double>)
Q_DECLARE_METATYPE(QVector<QString>)

void tst_QObjectBridge::callQtInvokable()
{
    qRegisterMetaType<QObjectList>();

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokable()"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 0);
    QCOMPARE(m_myObject->qtFunctionActuals(), QVariantList());

    // extra arguments should silently be ignored
    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokable(10, 20, 30)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 0);
    QCOMPARE(m_myObject->qtFunctionActuals(), QVariantList());

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithIntArg(123)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toInt(), 123);

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithIntArg('123')"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toInt(), 123);

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithLonglongArg(123)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 2);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toLongLong(), qlonglong(123));

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithFloatArg(123.5)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 3);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toDouble(), 123.5);

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithDoubleArg(123.5)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 4);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toDouble(), 123.5);

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithDoubleArg(new Number(1234.5))"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 4);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toDouble(), 1234.5);

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithBoolArg(new Boolean(true))"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 52);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toBool(), true);

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithStringArg('ciao')"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 5);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toString(), QLatin1String("ciao"));

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithStringArg(123)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 5);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toString(), QLatin1String("123"));

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithStringArg(null)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 5);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toString(), QString());
    QVERIFY(m_myObject->qtFunctionActuals().at(0).toString().isEmpty());

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithStringArg(undefined)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 5);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toString(), QString());
    QVERIFY(m_myObject->qtFunctionActuals().at(0).toString().isEmpty());

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithIntArgs(123, 456)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 6);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 2);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toInt(), 123);
    QCOMPARE(m_myObject->qtFunctionActuals().at(1).toInt(), 456);

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.myInvokableReturningInt()"), QLatin1String("123"));
    QCOMPARE(m_myObject->qtFunctionInvoked(), 7);
    QCOMPARE(m_myObject->qtFunctionActuals(), QVariantList());

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.myInvokableReturningLongLong()"), QLatin1String("456"));
    QCOMPARE(m_myObject->qtFunctionInvoked(), 39);
    QCOMPARE(m_myObject->qtFunctionActuals(), QVariantList());

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.myInvokableReturningString()"), QLatin1String("ciao"));
    QCOMPARE(m_myObject->qtFunctionInvoked(), 8);
    QCOMPARE(m_myObject->qtFunctionActuals(), QVariantList());

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithIntArg(123, 456)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 9);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 2);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toInt(), 123);
    QCOMPARE(m_myObject->qtFunctionActuals().at(1).toInt(), 456);

    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("typeof myObject.myInvokableWithVoidStarArg(null)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 44);
    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QString ret = evalJS("myObject.myInvokableWithVoidStarArg(123)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: incompatible type of argument(s) in call to myInvokableWithVoidStarArg(); candidates were\n    myInvokableWithVoidStarArg(void*)"));
        QCOMPARE(m_myObject->qtFunctionInvoked(), -1);
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QString ret = evalJS("myObject.myInvokableWithAmbiguousArg(123)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: ambiguous call of overloaded function myInvokableWithAmbiguousArg(); candidates were\n    myInvokableWithAmbiguousArg(int)\n    myInvokableWithAmbiguousArg(uint)"));
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QString ret = evalJS("myObject.myInvokableWithDefaultArgs(123, 'hello')", type);
        QCOMPARE(type, sUndefined);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 47);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 2);
        QCOMPARE(m_myObject->qtFunctionActuals().at(0).toInt(), 123);
        QCOMPARE(m_myObject->qtFunctionActuals().at(1).toString(), QLatin1String("hello"));
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QString ret = evalJS("myObject.myInvokableWithDefaultArgs(456)", type);
        QCOMPARE(type, sUndefined);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 47);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 2);
        QCOMPARE(m_myObject->qtFunctionActuals().at(0).toInt(), 456);
        QCOMPARE(m_myObject->qtFunctionActuals().at(1).toString(), QString());
    }

    // calling function that returns (const)ref
    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QString ret = evalJS("typeof myObject.myInvokableReturningRef()");
        QCOMPARE(ret, sUndefined);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 48);
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QString ret = evalJS("typeof myObject.myInvokableReturningConstRef()");
        QCOMPARE(ret, sUndefined);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 49);
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QVariant ret = evalJSV("myObject.myInvokableReturningQObjectStar()", type);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 13);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 0);
        QCOMPARE(type, sObject);
        QCOMPARE(ret.userType(), int(QMetaType::QObjectStar));
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QVariant ret = evalJSV("myObject.myInvokableWithQObjectListArg([myObject])", type);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 14);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
        QCOMPARE(type, sArray);
        QCOMPARE(ret.userType(), int(QVariant::List)); // All lists get downgraded to QVariantList
        QVariantList vl = qvariant_cast<QVariantList>(ret);
        QCOMPARE(vl.count(), 1);
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        m_myObject->setVariantProperty(QVariant(123));
        QVariant ret = evalJSV("myObject.myInvokableWithVariantArg(myObject.variantProperty)", type);
        QCOMPARE(type, sNumber);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 15);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
        QCOMPARE(m_myObject->qtFunctionActuals().at(0), m_myObject->variantProperty());
        QCOMPARE(ret.userType(), int(QMetaType::Double)); // all JS numbers are doubles, even though this started as an int
        QCOMPARE(ret.toInt(), 123);
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QVariant ret = evalJSV("myObject.myInvokableWithVariantArg(null)", type);
        QCOMPARE(type, sObject);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 15);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
        QCOMPARE(m_myObject->qtFunctionActuals().at(0), QVariant());
        QVERIFY(!m_myObject->qtFunctionActuals().at(0).isValid());
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QVariant ret = evalJSV("myObject.myInvokableWithVariantArg(undefined)", type);
        QCOMPARE(type, sObject);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 15);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
        QCOMPARE(m_myObject->qtFunctionActuals().at(0), QVariant());
        QVERIFY(!m_myObject->qtFunctionActuals().at(0).isValid());
    }

    /* XFAIL - variant support
    m_myObject->resetQtFunctionInvoked();
    {
        m_myObject->setVariantProperty(QVariant::fromValue(QBrush()));
        QVariant ret = evalJS("myObject.myInvokableWithVariantArg(myObject.variantProperty)");
        QVERIFY(ret.isVariant());
        QCOMPARE(m_myObject->qtFunctionInvoked(), 15);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
        QCOMPARE(ret.toVariant(), m_myObject->qtFunctionActuals().at(0));
        QCOMPARE(ret.toVariant(), m_myObject->variantProperty());
    }
    */

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QVariant ret = evalJSV("myObject.myInvokableWithVariantArg(123)", type);
        QCOMPARE(type, sNumber);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 15);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
        QCOMPARE(m_myObject->qtFunctionActuals().at(0), QVariant(123));
        QCOMPARE(ret.userType(), int(QMetaType::Double));
        QCOMPARE(ret.toInt(), 123);
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QVariant ret = evalJSV("myObject.myInvokableWithVariantMapArg({ a:123, b:'ciao' })", type);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 16);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);

        QVariant v = m_myObject->qtFunctionActuals().at(0);
        QCOMPARE(v.userType(), int(QMetaType::QVariantMap));

        QVariantMap vmap = qvariant_cast<QVariantMap>(v);
        QCOMPARE(vmap.keys().size(), 2);
        QCOMPARE(vmap.keys().at(0), QLatin1String("a"));
        QCOMPARE(vmap.value("a"), QVariant(123));
        QCOMPARE(vmap.keys().at(1), QLatin1String("b"));
        QCOMPARE(vmap.value("b"), QVariant("ciao"));

        QCOMPARE(type, sObject);

        QCOMPARE(ret.userType(), int(QMetaType::QVariantMap));
        vmap = qvariant_cast<QVariantMap>(ret);
        QCOMPARE(vmap.keys().size(), 2);
        QCOMPARE(vmap.keys().at(0), QLatin1String("a"));
        QCOMPARE(vmap.value("a"), QVariant(123));
        QCOMPARE(vmap.keys().at(1), QLatin1String("b"));
        QCOMPARE(vmap.value("b"), QVariant("ciao"));
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QVariant ret = evalJSV("myObject.myInvokableWithListOfIntArg([1, 5])", type);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 17);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
        QVariant v = m_myObject->qtFunctionActuals().at(0);
        QCOMPARE(v.userType(), qMetaTypeId<QList<int> >());
        QList<int> ilst = qvariant_cast<QList<int> >(v);
        QCOMPARE(ilst.size(), 2);
        QCOMPARE(ilst.at(0), 1);
        QCOMPARE(ilst.at(1), 5);

        QCOMPARE(type, sArray);
        QCOMPARE(ret.userType(), int(QMetaType::QVariantList)); // ints get converted to doubles, so this is a qvariantlist
        QVariantList vlst = qvariant_cast<QVariantList>(ret);
        QCOMPARE(vlst.size(), 2);
        QCOMPARE(vlst.at(0).toInt(), 1);
        QCOMPARE(vlst.at(1).toInt(), 5);
    }

    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QVariant ret = evalJSV("myObject.myInvokableWithQObjectStarArg(myObject)", type);
        QCOMPARE(m_myObject->qtFunctionInvoked(), 18);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
        QVariant v = m_myObject->qtFunctionActuals().at(0);
        QCOMPARE(v.userType(), int(QMetaType::QObjectStar));
        QCOMPARE(qvariant_cast<QObject*>(v), (QObject*)m_myObject);

        QCOMPARE(ret.userType(), int(QMetaType::QObjectStar));
        QCOMPARE(qvariant_cast<QObject*>(ret), (QObject*)m_myObject);

        QCOMPARE(type, sObject);
    }

    m_myObject->resetQtFunctionInvoked();
    {
        // no implicit conversion from integer to QObject*
        QString type;
        evalJS("myObject.myInvokableWithQObjectStarArg(123)", type);
        QCOMPARE(type, sError);
    }

    /*
    m_myObject->resetQtFunctionInvoked();
    {
        QString fun = evalJS("myObject.myInvokableWithQBrushArg");
        Q_ASSERT(fun.isFunction());
        QColor color(10, 20, 30, 40);
        // QColor should be converted to a QBrush
        QVariant ret = fun.call(QString(), QStringList()
                                    << qScriptValueFromValue(m_engine, color));
        QCOMPARE(m_myObject->qtFunctionInvoked(), 19);
        QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
        QVariant v = m_myObject->qtFunctionActuals().at(0);
        QCOMPARE(v.userType(), int(QMetaType::QBrush));
        QCOMPARE(qvariant_cast<QColor>(v), color);

        QCOMPARE(qscriptvalue_cast<QColor>(ret), color);
    }
    */

    // private slots should not be part of the QObject binding
    QCOMPARE(evalJS("typeof myObject.myPrivateSlot"), sUndefined);

    // protected slots should be fine
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myProtectedSlot()");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 36);

    // call with too few arguments
    {
        QString type;
        QString ret = evalJS("myObject.myInvokableWithIntArg()", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: too few arguments in call to myInvokableWithIntArg(); candidates are\n    myInvokableWithIntArg(int,int)\n    myInvokableWithIntArg(int)"));
    }

    // call function where not all types have been registered
    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QString ret = evalJS("myObject.myInvokableWithBrushStyleArg(0)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: cannot call myInvokableWithBrushStyleArg(): unknown type `Qt::BrushStyle'"));
        QCOMPARE(m_myObject->qtFunctionInvoked(), -1);
    }

    // call function with incompatible argument type
    m_myObject->resetQtFunctionInvoked();
    {
        QString type;
        QString ret = evalJS("myObject.myInvokableWithQBrushArg(null)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: incompatible type of argument(s) in call to myInvokableWithQBrushArg(); candidates were\n    myInvokableWithQBrushArg(QBrush)"));
        QCOMPARE(m_myObject->qtFunctionInvoked(), -1);
    }
}

void tst_QObjectBridge::connectAndDisconnect()
{
    // connect(function)
    QCOMPARE(evalJS("typeof myObject.mySignal"), sFunction);
    QCOMPARE(evalJS("typeof myObject.mySignal.connect"), sFunction);
    QCOMPARE(evalJS("typeof myObject.mySignal.disconnect"), sFunction);

    {
        QString type;
        evalJS("myObject.mySignal.connect(123)", type);
        QCOMPARE(type, sError);
    }

    evalJS("myHandler = function() { window.gotSignal = true; window.signalArgs = arguments; window.slotThisObject = this; }");

    QCOMPARE(evalJS("myObject.mySignal.connect(myHandler)"), sUndefined);

    evalJS("gotSignal = false");
    evalJS("myObject.mySignal()");
    QCOMPARE(evalJS("gotSignal"), sTrue);
    QCOMPARE(evalJS("signalArgs.length == 0"), sTrue);
    QCOMPARE(evalJS("slotThisObject == window"), sTrue);

    evalJS("gotSignal = false");
    m_myObject->emitMySignal();
    QCOMPARE(evalJS("gotSignal"), sTrue);
    QCOMPARE(evalJS("signalArgs.length == 0"), sTrue);

    QCOMPARE(evalJS("myObject.mySignalWithIntArg.connect(myHandler)"), sUndefined);

    evalJS("gotSignal = false");
    m_myObject->emitMySignalWithIntArg(123);
    QCOMPARE(evalJS("gotSignal"), sTrue);
    QCOMPARE(evalJS("signalArgs.length == 1"), sTrue);
    QCOMPARE(evalJS("signalArgs[0] == 123.0"), sTrue);

    QCOMPARE(evalJS("myObject.mySignal.disconnect(myHandler)"), sUndefined);
    {
        QString type;
        evalJS("myObject.mySignal.disconnect(myHandler)", type);
        QCOMPARE(type, sError);
    }

    evalJS("gotSignal = false");
    QCOMPARE(evalJS("myObject.mySignal2.connect(myHandler)"), sUndefined);
    m_myObject->emitMySignal2(true);
    QCOMPARE(evalJS("gotSignal"), sTrue);
    QCOMPARE(evalJS("signalArgs.length == 1"), sTrue);
    QCOMPARE(evalJS("signalArgs[0]"), sTrue);

    QCOMPARE(evalJS("myObject.mySignal2.disconnect(myHandler)"), sUndefined);

    QCOMPARE(evalJS("typeof myObject['mySignal2()']"), sFunction);
    QCOMPARE(evalJS("typeof myObject['mySignal2()'].connect"), sFunction);
    QCOMPARE(evalJS("typeof myObject['mySignal2()'].disconnect"), sFunction);

    QCOMPARE(evalJS("myObject['mySignal2()'].connect(myHandler)"), sUndefined);

    evalJS("gotSignal = false");
    m_myObject->emitMySignal2();
    QCOMPARE(evalJS("gotSignal"), sTrue);

    QCOMPARE(evalJS("myObject['mySignal2()'].disconnect(myHandler)"), sUndefined);

    // connect(object, function)
    evalJS("otherObject = { name:'foo' }");
    QCOMPARE(evalJS("myObject.mySignal.connect(otherObject, myHandler)"), sUndefined);
    QCOMPARE(evalJS("myObject.mySignal.disconnect(otherObject, myHandler)"), sUndefined);
    evalJS("gotSignal = false");
    m_myObject->emitMySignal();
    QCOMPARE(evalJS("gotSignal"), sFalse);

    {
        QString type;
        evalJS("myObject.mySignal.disconnect(otherObject, myHandler)", type);
        QCOMPARE(type, sError);
    }

    QCOMPARE(evalJS("myObject.mySignal.connect(otherObject, myHandler)"), sUndefined);
    evalJS("gotSignal = false");
    m_myObject->emitMySignal();
    QCOMPARE(evalJS("gotSignal"), sTrue);
    QCOMPARE(evalJS("signalArgs.length == 0"), sTrue);
    QCOMPARE(evalJS("slotThisObject"), evalJS("otherObject"));
    QCOMPARE(evalJS("slotThisObject.name"), QLatin1String("foo"));
    QCOMPARE(evalJS("myObject.mySignal.disconnect(otherObject, myHandler)"), sUndefined);

    evalJS("yetAnotherObject = { name:'bar', func : function() { } }");
    QCOMPARE(evalJS("myObject.mySignal2.connect(yetAnotherObject, myHandler)"), sUndefined);
    evalJS("gotSignal = false");
    m_myObject->emitMySignal2(true);
    QCOMPARE(evalJS("gotSignal"), sTrue);
    QCOMPARE(evalJS("signalArgs.length == 1"), sTrue);
    QCOMPARE(evalJS("slotThisObject == yetAnotherObject"), sTrue);
    QCOMPARE(evalJS("slotThisObject.name"), QLatin1String("bar"));
    QCOMPARE(evalJS("myObject.mySignal2.disconnect(yetAnotherObject, myHandler)"), sUndefined);

    QCOMPARE(evalJS("myObject.mySignal2.connect(myObject, myHandler)"), sUndefined);
    evalJS("gotSignal = false");
    m_myObject->emitMySignal2(true);
    QCOMPARE(evalJS("gotSignal"), sTrue);
    QCOMPARE(evalJS("signalArgs.length == 1"), sTrue);
    QCOMPARE(evalJS("slotThisObject == myObject"), sTrue);
    QCOMPARE(evalJS("myObject.mySignal2.disconnect(myObject, myHandler)"), sUndefined);

    // connect(obj, string)
    {
        QString type;
        QCOMPARE(evalJS("myObject.mySignal.connect(yetAnotherObject, 'func')", type), sUndefined);
        QCOMPARE(evalJS("myObject.mySignal.connect(myObject, 'mySlot')", type), sUndefined);
        QCOMPARE(evalJS("myObject.mySignal.disconnect(yetAnotherObject, 'func')", type), sUndefined);
        QCOMPARE(evalJS("myObject.mySignal.disconnect(myObject, 'mySlot')", type), sUndefined);
    }

    // check that emitting signals from script works

    // no arguments
    QCOMPARE(evalJS("myObject.mySignal.connect(myObject.mySlot)"), sUndefined);
    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.mySignal()"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 20);
    QCOMPARE(evalJS("myObject.mySignal.disconnect(myObject.mySlot)"), sUndefined);

    // one argument
    QCOMPARE(evalJS("myObject.mySignalWithIntArg.connect(myObject.mySlotWithIntArg)"), sUndefined);
    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.mySignalWithIntArg(123)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 21);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toInt(), 123);
    QCOMPARE(evalJS("myObject.mySignalWithIntArg.disconnect(myObject.mySlotWithIntArg)"), sUndefined);

    QCOMPARE(evalJS("myObject.mySignalWithIntArg.connect(myObject.mySlotWithDoubleArg)"), sUndefined);
    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.mySignalWithIntArg(123)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 22);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toDouble(), 123.0);
    QCOMPARE(evalJS("myObject.mySignalWithIntArg.disconnect(myObject.mySlotWithDoubleArg)"), sUndefined);

    QCOMPARE(evalJS("myObject.mySignalWithIntArg.connect(myObject.mySlotWithStringArg)"), sUndefined);
    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.mySignalWithIntArg(123)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 23);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toString(), QLatin1String("123"));
    QCOMPARE(evalJS("myObject.mySignalWithIntArg.disconnect(myObject.mySlotWithStringArg)"), sUndefined);

    // connecting to overloaded slot
    QCOMPARE(evalJS("myObject.mySignalWithIntArg.connect(myObject.myOverloadedSlot)"), sUndefined);
    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.mySignalWithIntArg(123)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 26); // double overload
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toInt(), 123);
    QCOMPARE(evalJS("myObject.mySignalWithIntArg.disconnect(myObject.myOverloadedSlot)"), sUndefined);

    QCOMPARE(evalJS("myObject.mySignalWithIntArg.connect(myObject['myOverloadedSlot(int)'])"), sUndefined);
    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.mySignalWithIntArg(456)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 28); // int overload
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toInt(), 456);
    QCOMPARE(evalJS("myObject.mySignalWithIntArg.disconnect(myObject['myOverloadedSlot(int)'])"), sUndefined);

    QCOMPARE(evalJS("myObject.mySignalWithIntArg.connect(myObject, 'myOverloadedSlot(int)')"), sUndefined);
    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.mySignalWithIntArg(456)"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 28); // int overload
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toInt(), 456);
    QCOMPARE(evalJS("myObject.mySignalWithIntArg.disconnect(myObject, 'myOverloadedSlot(int)')"), sUndefined);

    // erroneous input
    {
        // ### QtScript adds .connect to all functions, WebKit does only to signals/slots
        QString type;
        QString ret = evalJS("(function() { }).connect()", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("TypeError: 'undefined' is not a function (evaluating '(function() { }).connect()')"));
    }
    {
        QString type;
        QString ret = evalJS("var o = { }; o.connect = Function.prototype.connect;  o.connect()", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("TypeError: 'undefined' is not a function (evaluating 'o.connect()')"));
    }

    {
        QString type;
        QString ret = evalJS("(function() { }).connect(123)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("TypeError: 'undefined' is not a function (evaluating '(function() { }).connect(123)')"));
    }
    {
        QString type;
        QString ret = evalJS("var o = { }; o.connect = Function.prototype.connect;  o.connect(123)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("TypeError: 'undefined' is not a function (evaluating 'o.connect(123)')"));
    }

    {
        QString type;
        QString ret = evalJS("myObject.myInvokable.connect(123)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.connect: MyQObject::myInvokable() is not a signal"));
    }
    {
        QString type;
        QString ret = evalJS("myObject.myInvokable.connect(function() { })", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.connect: MyQObject::myInvokable() is not a signal"));
    }

    {
        QString type;
        QString ret = evalJS("myObject.mySignal.connect(123)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.connect: target is not a function"));
    }

    {
        QString type;
        QString ret = evalJS("var randomObject = new Object; myObject.mySignal.connect(myObject, randomObject)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.connect: target is not a function"));
    }

    {
        QString type;
        QString ret = evalJS("myObject.mySignal.connect(myObject, 'nonExistantSlot')", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.connect: target is not a function"));
    }

    {
        QString type;
        QString ret = evalJS("myObject.mySignal.disconnect()", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.disconnect: no arguments given"));
    }
    {
        QString type;
        QString ret = evalJS("var o = { }; o.disconnect = myObject.mySignal.disconnect;  o.disconnect()", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.disconnect: no arguments given"));
    }

    {
        QString type;
        QString ret = evalJS("myObject.mySignal.disconnect(myObject, 'nonExistantSlot')", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.disconnect: target is not a function"));
    }

    /* XFAIL - Function.prototype doesn't get connect/disconnect, just signals/slots
    {
        QString type;
        QString ret = evalJS("(function() { }).disconnect(123)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("TypeError: QtMetaMethod.disconnect: this object is not a signal"));
    }
    */

    {
        QString type;
        QString ret = evalJS("var o = { }; o.disconnect = myObject.myInvokable.disconnect; o.disconnect(123)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.disconnect: MyQObject::myInvokable() is not a signal"));
    }

    {
        QString type;
        QString ret = evalJS("myObject.myInvokable.disconnect(123)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.disconnect: MyQObject::myInvokable() is not a signal"));
    }
    {
        QString type;
        QString ret = evalJS("myObject.myInvokable.disconnect(function() { })", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.disconnect: MyQObject::myInvokable() is not a signal"));
    }

    {
        QString type;
        QString ret = evalJS("myObject.mySignal.disconnect(123)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.disconnect: target is not a function"));
    }

    {
        QString type;
        QString ret = evalJS("myObject.mySignal.disconnect(function() { })", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: QtMetaMethod.disconnect: failed to disconnect from MyQObject::mySignal()"));
    }

    // when the wrapper dies, the connection stays alive
    QCOMPARE(evalJS("myObject.mySignal.connect(myObject.mySlot)"), sUndefined);
    m_myObject->resetQtFunctionInvoked();
    m_myObject->emitMySignal();
    QCOMPARE(m_myObject->qtFunctionInvoked(), 20);
    evalJS("myObject = null");
    evalJS("gc()");
    m_myObject->resetQtFunctionInvoked();
    m_myObject->emitMySignal();
    QCOMPARE(m_myObject->qtFunctionInvoked(), 20);
}

void tst_QObjectBridge::overrideInvokable()
{
    m_myObject->resetQtFunctionInvoked();
    QCOMPARE(evalJS("myObject.myInvokable()"), sUndefined);
    QCOMPARE(m_myObject->qtFunctionInvoked(), 0);

    /* XFAIL - can't write to functions with RuntimeObject
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myInvokable = function() { window.a = 123; }");
    evalJS("myObject.myInvokable()");
    QCOMPARE(m_myObject->qtFunctionInvoked(), -1);
    QCOMPARE(evalJS("window.a").toDouble(), 123.0);

    evalJS("myObject.myInvokable = function() { window.a = 456; }");
    evalJS("myObject.myInvokable()");
    QCOMPARE(m_myObject->qtFunctionInvoked(), -1);
    QCOMPARE(evalJS("window.a").toDouble(), 456.0);
    */

    evalJS("delete myObject.myInvokable");
    evalJS("myObject.myInvokable()");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 0);

    /* XFAIL - ditto
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myInvokable = myObject.myInvokableWithIntArg");
    evalJS("myObject.myInvokable(123)");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 1);
    */

    evalJS("delete myObject.myInvokable");
    m_myObject->resetQtFunctionInvoked();
    // this form (with the '()') is read-only
    evalJS("myObject['myInvokable()'] = function() { window.a = 123; }");
    evalJS("myObject.myInvokable()");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 0);
}

void tst_QObjectBridge::overloadedSlots()
{
    // should pick myOverloadedSlot(double)
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myOverloadedSlot(10)");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 26);

    // should pick myOverloadedSlot(double)
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myOverloadedSlot(10.0)");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 26);

    // should pick myOverloadedSlot(QString)
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myOverloadedSlot('10')");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 29);

    // should pick myOverloadedSlot(bool)
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myOverloadedSlot(true)");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 25);

    // should pick myOverloadedSlot(QDateTime)
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myOverloadedSlot(new Date())");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 32);

    // should pick myOverloadedSlot(QVariant)
    /* XFAIL
    m_myObject->resetQtFunctionInvoked();
    QString f = evalJS("myObject.myOverloadedSlot");
    f.call(QString(), QStringList() << m_engine->newVariant(QVariant("ciao")));
    QCOMPARE(m_myObject->qtFunctionInvoked(), 35);
    */

    // Should pick myOverloadedSlot(QWebElement).
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myOverloadedSlot(document.body)");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 36);

    // should pick myOverloadedSlot(QObject*)
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myOverloadedSlot(myObject)");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 41);

    // should pick myOverloadedSlot(QObject*)
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myOverloadedSlot(null)");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 41);

    // should pick myOverloadedSlot(QStringList)
    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myOverloadedSlot(['hello'])");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 42);
}

class MyEnumTestQObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString p1 READ p1)
    Q_PROPERTY(QString p2 READ p2)
    Q_PROPERTY(QString p3 READ p3 SCRIPTABLE false)
    Q_PROPERTY(QString p4 READ p4)
    Q_PROPERTY(QString p5 READ p5 SCRIPTABLE false)
    Q_PROPERTY(QString p6 READ p6)
public:
    MyEnumTestQObject(QObject* parent = 0)
        : QObject(parent) { }

    QString p1() const { return QLatin1String("p1"); }
    QString p2() const { return QLatin1String("p2"); }
    QString p3() const { return QLatin1String("p3"); }
    QString p4() const { return QLatin1String("p4"); }
    QString p5() const { return QLatin1String("p5"); }
    QString p6() const { return QLatin1String("p6"); }

public Q_SLOTS:
    void mySlot() { }
    void myOtherSlot() { }
Q_SIGNALS:
    void mySignal();
};

void tst_QObjectBridge::enumerate_data()
{
    QTest::addColumn<QStringList>("expectedNames");

    QTest::newRow("enumerate all")
    << (QStringList()
        // meta-object-defined properties:
        //   inherited
        << "objectName"
        //   non-inherited
        << "p1" << "p2" << "p4" << "p6"
        // dynamic properties
        << "dp1" << "dp2" << "dp3"
        // inherited signals and slots
        << "destroyed(QObject*)" << "destroyed()"
        << "objectNameChanged(QString)"
        << "deleteLater()"
        // not included because it's private:
        // << "_q_reregisterTimers(void*)"
        // signals
        << "mySignal()"
        // slots
        << "mySlot()" << "myOtherSlot()");
}

void tst_QObjectBridge::enumerate()
{
    QFETCH(QStringList, expectedNames);

    MyEnumTestQObject enumQObject;
    // give it some dynamic properties
    enumQObject.setProperty("dp1", "dp1");
    enumQObject.setProperty("dp2", "dp2");
    enumQObject.setProperty("dp3", "dp3");
    m_page->mainFrame()->addToJavaScriptWindowObject("myEnumObject", &enumQObject);

    // enumerate in script
    {
        evalJS("var enumeratedProperties = []");
        evalJS("for (var p in myEnumObject) { enumeratedProperties.push(p); }");
        QStringList result = evalJSV("enumeratedProperties").toStringList();
        QCOMPARE(result.size(), expectedNames.size());
        for (int i = 0; i < expectedNames.size(); ++i)
            QCOMPARE(result.at(i), expectedNames.at(i));
    }
}

void tst_QObjectBridge::objectDeleted()
{
    MyQObject* qobj = new MyQObject();
    m_page->mainFrame()->addToJavaScriptWindowObject("bar", qobj);
    evalJS("bar.objectName = 'foo';");
    QCOMPARE(qobj->objectName(), QLatin1String("foo"));
    evalJS("bar.intProperty = 123;");
    QCOMPARE(qobj->intProperty(), 123);
    qobj->resetQtFunctionInvoked();
    evalJS("bar.myInvokable.call(bar);");
    QCOMPARE(qobj->qtFunctionInvoked(), 0);

    // do this, to ensure that we cache that it implements call
    evalJS("bar()");

    // now delete the object
    delete qobj;

    QCOMPARE(evalJS("typeof bar"), sObject);

    // any attempt to access properties of the object should result in an exception
    {
        QString type;
        QString ret = evalJS("bar.objectName", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: cannot access member `objectName' of deleted QObject"));
    }
    {
        QString type;
        QString ret = evalJS("bar.objectName = 'foo'", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: cannot access member `objectName' of deleted QObject"));
    }

    // myInvokable is stored in member table (since we've accessed it before deletion)
    {
        QString type;
        evalJS("bar.myInvokable", type);
        QCOMPARE(type, sFunction);
    }

    {
        QString type;
        QString ret = evalJS("bar.myInvokable.call(bar);", type);
        ret = evalJS("bar.myInvokable(bar)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: cannot call function of deleted QObject"));
    }
    // myInvokableWithIntArg is not stored in member table (since we've not accessed it)
    {
        QString type;
        QString ret = evalJS("bar.myInvokableWithIntArg", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: cannot access member `myInvokableWithIntArg' of deleted QObject"));
    }

    // access from script
    evalJS("window.o = bar;");
    {
        QString type;
        QString ret = evalJS("o.objectName", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: cannot access member `objectName' of deleted QObject"));
    }
    {
        QString type;
        QString ret = evalJS("o.myInvokable()", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: cannot call function of deleted QObject"));
    }
    {
        QString type;
        QString ret = evalJS("o.myInvokableWithIntArg(10)", type);
        QCOMPARE(type, sError);
        QCOMPARE(ret, QLatin1String("Error: cannot access member `myInvokableWithIntArg' of deleted QObject"));
    }
}

void tst_QObjectBridge::typeConversion()
{
    m_myObject->resetQtFunctionInvoked();

    QDateTime localdt(QDate(2008, 1, 18), QTime(12, 31, 0));
    QDateTime utclocaldt = localdt.toUTC();
    QDateTime utcdt(QDate(2008, 1, 18), QTime(12, 31, 0), Qt::UTC);

    // Dates in JS (default to local)
    evalJS("myObject.myOverloadedSlot(new Date(2008,0,18,12,31,0))");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 32);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toDateTime().toUTC(), utclocaldt);

    m_myObject->resetQtFunctionInvoked();
    evalJS("myObject.myOverloadedSlot(new Date(Date.UTC(2008,0,18,12,31,0)))");
    QCOMPARE(m_myObject->qtFunctionInvoked(), 32);
    QCOMPARE(m_myObject->qtFunctionActuals().size(), 1);
    QCOMPARE(m_myObject->qtFunctionActuals().at(0).toDateTime().toUTC(), utcdt);

    // Pushing QDateTimes into JS
    // Local
    evalJS("function checkDate(d) {window.__date_equals = (d.toString() == new Date(2008,0,18,12,31,0))?true:false;}");
    evalJS("myObject.mySignalWithDateTimeArg.connect(checkDate)");
    m_myObject->emitMySignalWithDateTimeArg(localdt);
    QCOMPARE(evalJS("window.__date_equals"), sTrue);
    evalJS("delete window.__date_equals");
    m_myObject->emitMySignalWithDateTimeArg(utclocaldt);
    QCOMPARE(evalJS("window.__date_equals"), sTrue);
    evalJS("delete window.__date_equals");
    evalJS("myObject.mySignalWithDateTimeArg.disconnect(checkDate); delete checkDate;");

    // UTC
    evalJS("function checkDate(d) {window.__date_equals = (d.toString() == new Date(Date.UTC(2008,0,18,12,31,0)))?true:false; }");
    evalJS("myObject.mySignalWithDateTimeArg.connect(checkDate)");
    m_myObject->emitMySignalWithDateTimeArg(utcdt);
    QCOMPARE(evalJS("window.__date_equals"), sTrue);
    evalJS("delete window.__date_equals");
    evalJS("myObject.mySignalWithDateTimeArg.disconnect(checkDate); delete checkDate;");
}

class StringListTestObject : public QObject {
    Q_OBJECT
public Q_SLOTS:
    QVariant stringList()
    {
        return QStringList() << "Q" << "t";
    };
};

void tst_QObjectBridge::arrayObjectEnumerable()
{
    QWebPage page;
    QWebFrame* frame = page.mainFrame();
    QObject* qobject = new StringListTestObject();
    frame->addToJavaScriptWindowObject("test", qobject, QWebFrame::ScriptOwnership);

    const QString script("var stringArray = test.stringList();"
                         "var result = '';"
                         "for (var i in stringArray) {"
                         "    result += stringArray[i];"
                         "}"
                         "result;");
    QCOMPARE(frame->evaluateJavaScript(script).toString(), QString::fromLatin1("Qt"));
}

void tst_QObjectBridge::domCycles()
{
    m_view->setHtml("<html><body>");
    QVariant v = m_page->mainFrame()->evaluateJavaScript("document");
    QVERIFY(v.type() == QVariant::Map);
}

void tst_QObjectBridge::jsByteArray()
{
    QByteArray ba("hello world");
    m_myObject->setByteArrayProperty(ba);

    // read-only property
    QCOMPARE(m_myObject->byteArrayProperty(), ba);
    QString type;
    QVariant v = evalJSV("myObject.byteArrayProperty");
    QCOMPARE(int(v.type()), int(QVariant::ByteArray));

    QCOMPARE(v.toByteArray(), ba);
}

void tst_QObjectBridge::ownership()
{
    // test ownership
    {
        QPointer<QObject> ptr = new QObject();
        QVERIFY(ptr);
        {
            QWebPage page;
            QWebFrame* frame = page.mainFrame();
            frame->addToJavaScriptWindowObject("test", ptr.data(), QWebFrame::ScriptOwnership);
        }
        QVERIFY(!ptr);
    }
    {
        QPointer<QObject> ptr = new QObject();
        QVERIFY(ptr);
        QObject* before = ptr.data();
        {
            QWebPage page;
            QWebFrame* frame = page.mainFrame();
            frame->addToJavaScriptWindowObject("test", ptr.data(), QWebFrame::QtOwnership);
        }
        QVERIFY(ptr.data() == before);
        delete ptr.data();
    }
    {
        QObject* parent = new QObject();
        QObject* child = new QObject(parent);
        QWebPage page;
        QWebFrame* frame = page.mainFrame();
        frame->addToJavaScriptWindowObject("test", child, QWebFrame::QtOwnership);
        QVariant v = frame->evaluateJavaScript("test");
        QCOMPARE(qvariant_cast<QObject*>(v), child);
        delete parent;
        v = frame->evaluateJavaScript("test");
        QCOMPARE(qvariant_cast<QObject*>(v), (QObject *)0);
    }
    {
        QPointer<QObject> ptr = new QObject();
        QVERIFY(ptr);
        {
            QWebPage page;
            QWebFrame* frame = page.mainFrame();
            frame->addToJavaScriptWindowObject("test", ptr.data(), QWebFrame::AutoOwnership);
        }
        // no parent, so it should be like ScriptOwnership
        QVERIFY(!ptr);
    }
    {
        QObject* parent = new QObject();
        QPointer<QObject> child = new QObject(parent);
        QVERIFY(child);
        {
            QWebPage page;
            QWebFrame* frame = page.mainFrame();
            frame->addToJavaScriptWindowObject("test", child.data(), QWebFrame::AutoOwnership);
        }
        // has parent, so it should be like QtOwnership
        QVERIFY(child);
        delete parent;
    }
}

void tst_QObjectBridge::nullValue()
{
    QVariant v = m_view->page()->mainFrame()->evaluateJavaScript("null");
    QVERIFY(v.isNull());
}

class TestFactory : public QObject {
    Q_OBJECT
public:
    TestFactory()
        : obj(0), counter(0)
    { }

    Q_INVOKABLE QObject* getNewObject()
    {
        delete obj;
        obj = new QObject(this);
        obj->setObjectName(QLatin1String("test") + QString::number(++counter));
        return obj;

    }

    QObject* obj;
    int counter;
};

void tst_QObjectBridge::qObjectWrapperWithSameIdentity()
{
    m_view->setHtml("<script>function triggerBug() { document.getElementById('span1').innerText = test.getNewObject().objectName; }</script>"
                    "<body><span id='span1'>test</span></body>");

    QWebFrame* mainFrame = m_view->page()->mainFrame();
    QCOMPARE(mainFrame->toPlainText(), QString("test"));

    mainFrame->addToJavaScriptWindowObject("test", new TestFactory, QWebFrame::ScriptOwnership);

    mainFrame->evaluateJavaScript("triggerBug();");
    QCOMPARE(mainFrame->toPlainText(), QString("test1"));

    mainFrame->evaluateJavaScript("triggerBug();");
    QCOMPARE(mainFrame->toPlainText(), QString("test2"));
}

void tst_QObjectBridge::introspectQtMethods_data()
{
    QTest::addColumn<QString>("objectExpression");
    QTest::addColumn<QString>("methodName");
    QTest::addColumn<QStringList>("expectedPropertyNames");

    QTest::newRow("myObject.mySignal")
        << "myObject" << "mySignal" << (QStringList() << "connect" << "disconnect" << "name");
    QTest::newRow("myObject.mySlot")
        << "myObject" << "mySlot" << (QStringList() << "connect" << "disconnect" << "name");
    QTest::newRow("myObject.myInvokable")
        << "myObject" << "myInvokable" << (QStringList() << "connect" << "disconnect" << "name");
    QTest::newRow("myObject.mySignal.connect")
        << "myObject.mySignal" << "connect" << (QStringList() << "name");
    QTest::newRow("myObject.mySignal.disconnect")
        << "myObject.mySignal" << "disconnect" << (QStringList() << "name");
}

void tst_QObjectBridge::introspectQtMethods()
{
    QFETCH(QString, objectExpression);
    QFETCH(QString, methodName);
    QFETCH(QStringList, expectedPropertyNames);

    QString methodLookup = QString::fromLatin1("%0['%1']").arg(objectExpression).arg(methodName);
    QCOMPARE(evalJSV(QString::fromLatin1("Object.getOwnPropertyNames(%0).sort()").arg(methodLookup)).toStringList(), expectedPropertyNames);

    for (int i = 0; i < expectedPropertyNames.size(); ++i) {
        QString name = expectedPropertyNames.at(i);
        QCOMPARE(evalJS(QString::fromLatin1("%0.hasOwnProperty('%1')").arg(methodLookup).arg(name)), sTrue);
        evalJS(QString::fromLatin1("var descriptor = Object.getOwnPropertyDescriptor(%0, '%1')").arg(methodLookup).arg(name));
        QCOMPARE(evalJS("typeof descriptor"), QString::fromLatin1("object"));
        QCOMPARE(evalJS("descriptor.get"), sUndefined);
        QCOMPARE(evalJS("descriptor.set"), sUndefined);
        QCOMPARE(evalJS(QString::fromLatin1("descriptor.value === %0['%1']").arg(methodLookup).arg(name)), sTrue);
        QCOMPARE(evalJS(QString::fromLatin1("descriptor.enumerable")), sFalse);
        QCOMPARE(evalJS(QString::fromLatin1("descriptor.configurable")), sFalse);
    }

    QVERIFY(evalJSV("var props=[]; for (var p in myObject.deleteLater) {props.push(p);}; props.sort()").toStringList().isEmpty());
}

void tst_QObjectBridge::webElementSlotOnly()
{
    MyWebElementSlotOnlyObject object;
    m_page->mainFrame()->setHtml("<html><head><body></body></html>");
    m_page->mainFrame()->addToJavaScriptWindowObject("myWebElementSlotObject", &object);
    evalJS("myWebElementSlotObject.doSomethingWithWebElement(document.body)");
    QCOMPARE(evalJS("myWebElementSlotObject.tagName"), QString("BODY"));
}

class TestPluginWidget : public QWidget {
    Q_OBJECT
public:
    TestPluginWidget() { }

public Q_SLOTS:
    int slotWithReturnValue() { return 42; }
};

class TestWebPage : public QWebPage {
    Q_OBJECT
public:
    TestWebPage(QObject* parent = 0)
        : QWebPage(parent)
        , creationCount(0)
    { }

    int creationCount;

protected:
    virtual QObject* createPlugin(const QString&, const QUrl&, const QStringList&, const QStringList&)
    {
        creationCount++;
        return new TestPluginWidget;
    }
};

void tst_QObjectBridge::scriptablePlugin()
{
    QWebView view;
    TestWebPage* page = new TestWebPage;
    view.setPage(page);
    page->setParent(&view);
    view.settings()->setAttribute(QWebSettings::PluginsEnabled, true);

    page->mainFrame()->setHtml("<object width=100 height=100 type=\"application/x-qt-plugin\"></object>");
    QCOMPARE(page->creationCount, 1);

    QVariant result = page->mainFrame()->evaluateJavaScript("document.querySelector(\"object\").slotWithReturnValue()");
    QCOMPARE(result.toString(), QLatin1String("42"));
}

QTEST_MAIN(tst_QObjectBridge)
#include "tst_qobjectbridge.moc"
