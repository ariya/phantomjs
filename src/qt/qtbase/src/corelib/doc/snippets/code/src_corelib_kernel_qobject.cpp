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
QLineEdit *lineEdit = static_cast<QLineEdit *>(
        qt_find_obj_child(myWidget, "QLineEdit", "my line edit"));
if (lineEdit)
    lineEdit->setText("Default");
//! [0]


//! [1]
QObject *obj = new QPushButton;
obj->metaObject()->className();             // returns "QPushButton"

QPushButton::staticMetaObject.className();  // returns "QPushButton"
//! [1]


//! [2]
QPushButton::staticMetaObject.className();  // returns "QPushButton"

QObject *obj = new QPushButton;
obj->metaObject()->className();             // returns "QPushButton"
//! [2]


//! [3]
QObject *obj = new QTimer;          // QTimer inherits QObject

QTimer *timer = qobject_cast<QTimer *>(obj);
// timer == (QObject *)obj

QAbstractButton *button = qobject_cast<QAbstractButton *>(obj);
// button == 0
//! [3]


//! [4]
QTimer *timer = new QTimer;         // QTimer inherits QObject
timer->inherits("QTimer");          // returns true
timer->inherits("QObject");         // returns true
timer->inherits("QAbstractButton"); // returns false

// QVBoxLayout inherits QObject and QLayoutItem
QVBoxLayout *layout = new QVBoxLayout;
layout->inherits("QObject");        // returns true
layout->inherits("QLayoutItem");    // returns true (even though QLayoutItem is not a QObject)
//! [4]


//! [5]
qDebug("MyClass::setPrecision(): (%s) invalid precision %f",
       qPrintable(objectName()), newPrecision);
//! [5]


//! [6]
class MainWindow : public QMainWindow
{
public:
    MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

private:
    QTextEdit *textEdit;
};

MainWindow::MainWindow()
{
    textEdit = new QTextEdit;
    setCentralWidget(textEdit);

    textEdit->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == textEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            qDebug() << "Ate key press" << keyEvent->key();
            return true;
        } else {
            return false;
        }
    } else {
        // pass the event on to the parent class
        return QMainWindow::eventFilter(obj, event);
    }
}
//! [6]


//! [7]
myObject->moveToThread(QApplication::instance()->thread());
//! [7]


//! [8]
class MyObject : public QObject
{
    Q_OBJECT

public:
    MyObject(QObject *parent = 0);

protected:
    void timerEvent(QTimerEvent *event);
};

MyObject::MyObject(QObject *parent)
    : QObject(parent)
{
    startTimer(50);     // 50-millisecond timer
    startTimer(1000);   // 1-second timer
    startTimer(60000);  // 1-minute timer
}

void MyObject::timerEvent(QTimerEvent *event)
{
    qDebug() << "Timer ID:" << event->timerId();
}
//! [8]


//! [9]
QList<QObject *> list = window()->queryList("QAbstractButton"));
foreach (QObject *obj, list)
    static_cast<QAbstractButton *>(obj)->setEnabled(false);
//! [9]


//! [10]
QPushButton *button = parentWidget->findChild<QPushButton *>("button1");
//! [10]


//! [11]
QListWidget *list = parentWidget->findChild<QListWidget *>();
//! [11]


//! [12]
QList<QWidget *> widgets = parentWidget.findChildren<QWidget *>("widgetname");
//! [12]


//! [13]
QList<QPushButton *> allPButtons = parentWidget.findChildren<QPushButton *>();
//! [13]


//! [14]
monitoredObj->installEventFilter(filterObj);
//! [14]


//! [15]
class KeyPressEater : public QObject
{
    Q_OBJECT
    ...

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

bool KeyPressEater::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        qDebug("Ate key press %d", keyEvent->key());
        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}
//! [15]


//! [16]
KeyPressEater *keyPressEater = new KeyPressEater(this);
QPushButton *pushButton = new QPushButton(this);
QListView *listView = new QListView(this);

pushButton->installEventFilter(keyPressEater);
listView->installEventFilter(keyPressEater);
//! [16]


//! [17]
MyWindow::MyWindow()
{
    QLabel *senderLabel = new QLabel(tr("Name:"));
    QLabel *recipientLabel = new QLabel(tr("Name:", "recipient"));
//! [17]
}


//! [18]
int n = messages.count();
showMessage(tr("%n message(s) saved", "", n));
//! [18]


//! [19]
n == 1 ? tr("%n message saved") : tr("%n messages saved")
//! [19]


//! [20]
label->setText(tr("F\374r \310lise"));
//! [20]


//! [21]
if (receivers(SIGNAL(valueChanged(QByteArray))) > 0) {
    QByteArray data;
    get_the_value(&data);       // expensive operation
    emit valueChanged(data);
}
//! [21]


//! [22]
QLabel *label = new QLabel;
QScrollBar *scrollBar = new QScrollBar;
QObject::connect(scrollBar, SIGNAL(valueChanged(int)),
                 label,  SLOT(setNum(int)));
//! [22]


//! [23]
// WRONG
QObject::connect(scrollBar, SIGNAL(valueChanged(int value)),
                 label, SLOT(setNum(int value)));
//! [23]


//! [24]
class MyWidget : public QWidget
{
    Q_OBJECT

public:
    MyWidget();

signals:
    void buttonClicked();

private:
    QPushButton *myButton;
};

MyWidget::MyWidget()
{
    myButton = new QPushButton(this);
    connect(myButton, SIGNAL(clicked()),
            this, SIGNAL(buttonClicked()));
}
//! [24]


//! [25]
QObject::connect: Cannot queue arguments of type 'MyType'
(Make sure 'MyType' is registered using qRegisterMetaType().)
//! [25]


//! [26]
disconnect(myObject, 0, 0, 0);
//! [26]


//! [27]
myObject->disconnect();
//! [27]


//! [28]
disconnect(myObject, SIGNAL(mySignal()), 0, 0);
//! [28]


//! [29]
myObject->disconnect(SIGNAL(mySignal()));
//! [29]


//! [30]
disconnect(myObject, 0, myReceiver, 0);
//! [30]


//! [31]
myObject->disconnect(myReceiver);
//! [31]


//! [32]
if (signal == QMetaMethod::fromSignal(&MyObject::valueChanged)) {
    // signal is valueChanged
}
//! [32]


//! [33]
void on_<object name>_<signal name>(<signal parameters>);
//! [33]


//! [34]
void on_button1_clicked();
//! [34]


//! [35]
class MyClass : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("Author", "Pierre Gendron")
    Q_CLASSINFO("URL", "http://www.my-organization.qc.ca")

public:
    ...
};
//! [35]

//! [37]
Q_PROPERTY(QString title READ title WRITE setTitle USER true)
//! [37]


//! [38]
class MyClass : public QObject
{
    Q_OBJECT
    Q_ENUMS(Priority)

public:
    MyClass(QObject *parent = 0);
    ~MyClass();

    enum Priority { High, Low, VeryHigh, VeryLow };
    void setPriority(Priority priority);
    Priority priority() const;
};
//! [38]


//! [39a]
class QLibrary : public QObject
{
    ...
    Q_FLAGS(LoadHint LoadHints)
    ...
//! [39a]

//! [39b]
    ...
public:
    enum LoadHint {
        ResolveAllSymbolsHint = 0x01,
        ExportExternalSymbolsHint = 0x02,
        LoadArchiveMemberHint = 0x04
    };
    Q_DECLARE_FLAGS(LoadHints, LoadHint)
    ...
//! [39b]


//! [40]
//: This name refers to a host name.
hostNameLabel->setText(tr("Name:"));

/*: This text refers to a C++ code example. */
QString example = tr("Example");
//! [40]

//! [41]
QPushButton *button = parentWidget->findChild<QPushButton *>("button1", Qt::FindDirectChildOnly);
//! [41]


//! [42]
QListWidget *list = parentWidget->findChild<QListWidget *>(QString(), Qt::FindDirectChildOnly);
//! [42]


//! [43]
QList<QPushButton *> childButtons = parentWidget.findChildren<QPushButton *>(QString(), Qt::FindDirectChildOnly);
//! [43]

//! [44]
QLabel *label = new QLabel;
QLineEdit *lineEdit = new QLineEdit;
QObject::connect(lineEdit, &QLineEdit::textChanged,
                 label,  &QLabel::setText);
//! [44]

//! [45]
void someFunction();
QPushButton *button = new QPushButton;
QObject::connect(button, &QPushButton::clicked, someFunction);
//! [45]

//! [46]
QByteArray page = ...;
QTcpSocket *socket = new QTcpSocket;
socket->connectToHost("qt-project.org", 80);
QObject::connect(socket, &QTcpSocket::connected, [=] () {
        socket->write("GET " + page + "\r\n");
    });
//! [46]

//! [47]
disconnect(myObject, &MyObject::mySignal(), 0, 0);
//! [47]

//! [48]
QObject::disconnect(lineEdit, &QLineEdit::textChanged,
                 label,  &QLabel::setText);
//! [48]

//! [49]
static const QMetaMethod valueChangedSignal = QMetaMethod::fromSignal(&MyObject::valueChanged);
if (isSignalConnected(valueChangedSignal)) {
    QByteArray data;
    data = get_the_value();       // expensive operation
    emit valueChanged(data);
}
//! [49]

//! [50]
void someFunction();
QPushButton *button = new QPushButton;
QObject::connect(button, &QPushButton::clicked, this, someFunction, Qt::QueuedConnection);
//! [50]

//! [51]
QByteArray page = ...;
QTcpSocket *socket = new QTcpSocket;
socket->connectToHost("qt-project.org", 80);
QObject::connect(socket, &QTcpSocket::connected, this, [=] () {
        socket->write("GET " + page + "\r\n");
    }, Qt::AutoConnection);
//! [51]

//! [meta data]
//: This is a comment for the translator.
//= qtn_foo_bar
//~ loc-layout_id foo_dialog
//~ loc-blank False
//~ magic-stuff This might mean something magic.
QString text = MyMagicClass::tr("Sim sala bim.");
//! [meta data]

//! [explicit tr context]
QString text = QScrollBar::tr("Page up");
//! [explicit tr context]
