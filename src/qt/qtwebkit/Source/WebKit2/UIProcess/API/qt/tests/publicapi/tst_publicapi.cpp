/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#include "config.h"

#include <QMetaEnum>
#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaType>
#include <QtTest/QtTest>
#include <private/qquickwebview_p.h>
#include <private/qwebloadrequest_p.h>
#include <private/qwebnavigationrequest_p.h>

class tst_publicapi : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void publicAPI();
};

static QList<const QMetaObject*> typesToCheck = QList<const QMetaObject*>()
    << &QQuickWebView::staticMetaObject
    << &QWebLoadRequest::staticMetaObject
    << &QWebNavigationRequest::staticMetaObject;

static QStringList expectedAPI = QStringList()
    << "QQuickWebView.AcceptRequest --> NavigationRequestAction"
    << "QQuickWebView.IgnoreRequest --> NavigationRequestAction"
    << "QQuickWebView.LoadStartedStatus --> LoadStatus"
    << "QQuickWebView.LoadStoppedStatus --> LoadStatus"
    << "QQuickWebView.LoadSucceededStatus --> LoadStatus"
    << "QQuickWebView.LoadFailedStatus --> LoadStatus"
    << "QQuickWebView.NoErrorDomain --> ErrorDomain"
    << "QQuickWebView.InternalErrorDomain --> ErrorDomain"
    << "QQuickWebView.NetworkErrorDomain --> ErrorDomain"
    << "QQuickWebView.HttpErrorDomain --> ErrorDomain"
    << "QQuickWebView.DownloadErrorDomain --> ErrorDomain"
    << "QQuickWebView.LinkClickedNavigation --> NavigationType"
    << "QQuickWebView.FormSubmittedNavigation --> NavigationType"
    << "QQuickWebView.BackForwardNavigation --> NavigationType"
    << "QQuickWebView.ReloadNavigation --> NavigationType"
    << "QQuickWebView.FormResubmittedNavigation --> NavigationType"
    << "QQuickWebView.OtherNavigation --> NavigationType"
    << "QQuickWebView.title --> QString"
    << "QQuickWebView.url --> QUrl"
    << "QQuickWebView.icon --> QUrl"
    << "QQuickWebView.canGoBack --> bool"
    << "QQuickWebView.canGoForward --> bool"
    << "QQuickWebView.loading --> bool"
    << "QQuickWebView.loadProgress --> int"
    << "QQuickWebView.titleChanged() --> void"
    << "QQuickWebView.navigationHistoryChanged() --> void"
    << "QQuickWebView.loadingChanged(QWebLoadRequest*) --> void"
    << "QQuickWebView.loadProgressChanged() --> void"
    << "QQuickWebView.urlChanged() --> void"
    << "QQuickWebView.iconChanged() --> void"
    << "QQuickWebView.linkHovered(QUrl,QString) --> void"
    << "QQuickWebView.navigationRequested(QWebNavigationRequest*) --> void"
    << "QQuickWebView.loadHtml(QString,QUrl,QUrl) --> void"
    << "QQuickWebView.loadHtml(QString,QUrl) --> void"
    << "QQuickWebView.loadHtml(QString) --> void"
    << "QQuickWebView.goBack() --> void"
    << "QQuickWebView.goForward() --> void"
    << "QQuickWebView.stop() --> void"
    << "QQuickWebView.reload() --> void"
    << "QWebLoadRequest.url --> QUrl"
    << "QWebLoadRequest.status --> QQuickWebView::LoadStatus"
    << "QWebLoadRequest.errorString --> QString"
    << "QWebLoadRequest.errorDomain --> QQuickWebView::ErrorDomain"
    << "QWebLoadRequest.errorCode --> int"
    << "QWebNavigationRequest.url --> QUrl"
    << "QWebNavigationRequest.mouseButton --> int"
    << "QWebNavigationRequest.keyboardModifiers --> int"
    << "QWebNavigationRequest.action --> QQuickWebView::NavigationRequestAction"
    << "QWebNavigationRequest.navigationType --> QQuickWebView::NavigationType"
    << "QWebNavigationRequest.actionChanged() --> void"
    ;

static bool isCheckedEnum(const QByteArray& typeName)
{
    QList<QByteArray> tokens = typeName.split(':');
    if (tokens.size() == 3) {
        QByteArray& enumClass = tokens[0];
        QByteArray& enumName = tokens[2];
        foreach (const QMetaObject* mo, typesToCheck) {
            if (mo->className() != enumClass)
                continue;
            for (int i = mo->enumeratorOffset(); i < mo->enumeratorCount(); ++i)
                if (mo->enumerator(i).name() == enumName)
                    return true;
        }
    }
    return false;
}

static bool isCheckedClass(const QByteArray& typeName)
{
    foreach (const QMetaObject* mo, typesToCheck) {
        QByteArray moTypeName(mo->className());
        if (moTypeName == typeName || moTypeName + "*" == typeName)
            return true;
    }
    return false;
}

static void checkKnownType(const QByteArray& typeName)
{
    if ((typeName != "void" && !QMetaType::type(typeName)) || QMetaType::type(typeName) >= QMetaType::User) {
        bool knownEnum = isCheckedEnum(typeName);
        bool knownClass = isCheckedClass(typeName);
        QVERIFY2(knownEnum || knownClass, qPrintable(QString("The API uses an unknown type [%1], you might have to add it to the typesToCheck list.").arg(typeName.constData())));
    }
}

static void gatherAPI(const QString& prefix, const QMetaEnum& metaEnum, QStringList* output)
{
    for (int i = 0; i < metaEnum.keyCount(); ++i)
        *output << QString::fromLatin1("%1%2 --> %3").arg(prefix).arg(metaEnum.key(i)).arg(metaEnum.name());
}

static void gatherAPI(const QString& prefix, const QMetaProperty& property, QStringList* output)
{
    *output << QString::fromLatin1("%1%2 --> %3").arg(prefix).arg(property.name()).arg(property.typeName());
    checkKnownType(property.typeName());
}

static void gatherAPI(const QString& prefix, const QMetaMethod& method, QStringList* output)
{
    if (method.access() != QMetaMethod::Private) {
        const char* methodTypeName = !!strlen(method.typeName()) ? method.typeName() : "void";
        *output << QString::fromLatin1("%1%2 --> %3").arg(prefix).arg(QString::fromLatin1(method.methodSignature())).arg(QString::fromLatin1(methodTypeName));

        checkKnownType(methodTypeName);
        foreach (QByteArray paramType, method.parameterTypes())
            checkKnownType(paramType);
    }
}

static void gatherAPI(const QString& prefix, const QMetaObject* meta, QStringList* output)
{
    // *Offset points us only at the leaf class members, we don't have inheritance in our API yet anyway.
    for (int i = meta->enumeratorOffset(); i < meta->enumeratorCount(); ++i)
        gatherAPI(prefix, meta->enumerator(i), output);
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i)
        gatherAPI(prefix, meta->property(i), output);
    for (int i = meta->methodOffset(); i < meta->methodCount(); ++i)
        gatherAPI(prefix, meta->method(i), output);
}

void tst_publicapi::publicAPI()
{
    QStringList actualAPI;
    foreach (const QMetaObject* meta, typesToCheck)
        gatherAPI(QString::fromLatin1(meta->className()) + ".", meta, &actualAPI);

    // Uncomment to print the actual API.
    // foreach(QString actual, actualAPI)
    //     printf("    << \"%s\"\n", qPrintable(actual));

    // Make sure that nothing slips in the public API unintentionally.
    foreach (QString actual, actualAPI)
        QVERIFY2(expectedAPI.contains(actual), qPrintable(actual));
    // Make sure that the expected list is up-to-date with intentionally added APIs.
    foreach (QString expected, expectedAPI)
        QVERIFY2(actualAPI.contains(expected), qPrintable(expected));
}

QTEST_MAIN(tst_publicapi)

#include "tst_publicapi.moc"
