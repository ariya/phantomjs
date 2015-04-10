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
class MainApplicationAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.DBus.MainApplication")
    Q_PROPERTY(QString caption READ caption WRITE setCaption)
    Q_PROPERTY(QString organizationName READ organizationName)
    Q_PROPERTY(QString organizationDomain READ organizationDomain)

private:
    QApplication *app;

public:
    MainApplicationAdaptor(QApplication *application)
        : QDBusAbstractAdaptor(application), app(application)
    {
        connect(application, SIGNAL(aboutToQuit()), SIGNAL(aboutToQuit()));
        connect(application, SIGNAL(focusChanged(QWidget*,QWidget*)),
                SLOT(focusChangedSlot(QWidget*,QWidget*)));
    }

    QString caption()
    {
        if (app->hasMainWindow())
            return app->mainWindow()->caption();
        return QString(""); // must not return a null QString
    }

    void setCaption(const QString &newCaption)
    {
        if (app->hasMainWindow())
            app->mainWindow()->setCaption(newCaption);
    }

    QString organizationName()
    {
        return app->organizationName();
    }

    QString organizationDomain()
    {
        return app->organizationDomain();
    }

public slots:
    Q_NOREPLY void quit()
    { app->quit(); }

    void reparseConfiguration()
    { app->reparseConfiguration(); }

    QString mainWindowObject()
    {
        if (app->hasMainWindow())
            return QString("/%1/mainwindow").arg(app->applicationName());
        return QString();
    }

    void setSessionManagement(bool enable)
    {
        if (enable)
           app->enableSessionManagement();
        else
           app->disableSessionManagement();
    }

private slots:
    void focusChangedSlot(QWidget *, QWidget *now)
    {
        if (now == app->mainWindow())
            emit mainWindowHasFocus();
    }

signals:
    void aboutToQuit();
    void mainWindowHasFocus();
};
//! [0]


//! [1]
interface org.kde.DBus.MainApplication
{
    property readwrite STRING caption
    property read STRING organizationName
    property read STRING organizationDomain

    method quit() annotation("org.freedesktop.DBus.Method.NoReply", "true")
    method reparseConfiguration()
    method mainWindowObject(out STRING)
    method disableSessionManagement(in BOOLEAN enable)

    signal aboutToQuit()
    signal mainWindowHasFocus()
}
//! [1]


//! [2]
int main(int argc, char **argv)
{
    // create the QApplication object
    QApplication app(argc, argv);

    // create the MainApplication adaptor:
    new MainApplicationAdaptor(app);

    // connect to D-Bus and register as an object:
    QDBusConnection::sessionBus().registerObject("/MainApplication", &app);

    // add main window, etc.
    [...]

    app.exec();
}
//! [2]


//! [3]
class MainApplicationAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.DBus.MainApplication")
//! [3]


//! [4]
    Q_PROPERTY(QString caption READ caption WRITE setCaption)
    Q_PROPERTY(QString organizationName READ organizationName)
    Q_PROPERTY(QString organizationDomain READ organizationDomain)
//! [4]


//! [5]
QString caption()
{
    if (app->hasMainWindow())
        return app->mainWindow()->caption();
    return QString();
}

void setCaption(const QString &newCaption)
{
    if (app->hasMainWindow())
        app->mainWindow()->setCaption(newCaption);
}

QString organizationName()
{
    return app->organizationName();
}

QString organizationDomain()
{
    return app->organizationDomain();
}
//! [5]


//! [6]
MyInterfaceAdaptor(QApplication *application)
    : QDBusAbstractAdaptor(application), app(application)
{
    connect(application, SIGNAL(aboutToQuit()), SIGNAL(aboutToQuit());
    connect(application, SIGNAL(focusChanged(QWidget*,QWidget*)),
            SLOT(focusChangedSlot(QWidget*,QWidget*)));
}
//! [6]


//! [7]
public slots:
    Q_NOREPLY void quit()
    { app->quit(); }

    void reparseConfiguration()
    { app->reparseConfiguration(); }

    QString mainWindowObject()
    {
        if (app->hasMainWindow())
            return QString("/%1/mainwindow").arg(app->applicationName());
        return QString();
    }

    void setSessionManagement(bool enable)
    {
        if (enable)
           app->enableSessionManagement();
        else
           app->disableSessionManagement();
    }
//! [7]


//! [8]
signals:
    void aboutToQuit();
    void mainWindowHasFocus();
//! [8]


//! [9]
private slots:
    void focusChangedSlot(QWidget *, QWidget *now)
    {
        if (now == app->mainWindow())
            emit mainWindowHasFocus();
    }
//! [9]


//! [10]
struct RequestData
{
    QString request;
    QString processedData;
    QDBusMessage reply;
};

QString processRequest(const QString &request, const QDBusMessage &message)
{
    RequestData *data = new RequestData;
    data->request = request;
    message.setDelayedReply(true);
    data->reply = message.createReply();
    QDBusConnection::sessionBus().send(data->reply);

    appendRequest(data);
    return QString();
}
//! [10]


//! [11]
void sendReply(RequestData *data)
{
    // data->processedData has been initialized with the request's reply
    QDBusMessage &reply = &data->reply;

    // send the reply over D-Bus:
    reply << data->processedData;
    QDBusConnection::sessionBus().send(reply);

    // dispose of the transaction data
    delete data;
}
//! [11]


//! [12]
Q_NOREPLY void myMethod();
//! [12]
