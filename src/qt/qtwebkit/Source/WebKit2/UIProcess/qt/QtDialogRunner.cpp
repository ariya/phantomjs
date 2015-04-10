/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "QtDialogRunner.h"

#include "WKRetainPtr.h"
#include "WKStringQt.h"
#include "qquickwebview_p_p.h"
#include "qwebpermissionrequest_p.h"
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <wtf/PassOwnPtr.h>

namespace WebKit {

QtDialogRunner::QtDialogRunner(QQuickWebView* webView)
    : QEventLoop()
    , m_webView(webView)
    , m_wasAccepted(false)
{
}

QtDialogRunner::~QtDialogRunner()
{
}

// All dialogs need a way to support the state of the
// dialog being done/finished/dismissed. This is handled
// in the dialog base context.
class DialogContextBase : public QObject {
    Q_OBJECT

public:
    DialogContextBase()
        : QObject()
        , m_dismissed(false)
    {
    }

public Q_SLOTS:
    // Allows clients to call dismiss() directly, while also
    // being able to hook up signals to automatically also
    // dismiss the dialog since it's a slot.

    void dismiss()
    {
        m_dismissed = true;
        emit dismissed();
    }

Q_SIGNALS:
    void dismissed();

private:
    // We store the dismissed state so that run() can check to see if a
    // dialog has already been dismissed before spinning an event loop.
    bool m_dismissed;
    friend void QtDialogRunner::run();
};

class DialogContextObject : public DialogContextBase {
    Q_OBJECT
    Q_PROPERTY(QString message READ message CONSTANT)
    Q_PROPERTY(QString defaultValue READ defaultValue CONSTANT)

public:
    DialogContextObject(const QString& message, const QString& defaultValue = QString())
        : DialogContextBase()
        , m_message(message)
        , m_defaultValue(defaultValue)
    {
        connect(this, SIGNAL(accepted(QString)), SLOT(dismiss()));
        connect(this, SIGNAL(rejected()), SLOT(dismiss()));
    }
    QString message() const { return m_message; }
    QString defaultValue() const { return m_defaultValue; }

public Q_SLOTS:
    void accept(const QString& result = QString()) { emit accepted(result); }
    void reject() { emit rejected(); }

Q_SIGNALS:
    void accepted(const QString& result);
    void rejected();

private:
    QString m_message;
    QString m_defaultValue;
};

class BaseAuthenticationContextObject : public DialogContextBase {
    Q_OBJECT
    Q_PROPERTY(QString hostname READ hostname CONSTANT)
    Q_PROPERTY(QString prefilledUsername READ prefilledUsername CONSTANT)

public:
    BaseAuthenticationContextObject(const QString& hostname, const QString& prefilledUsername)
        : DialogContextBase()
        , m_hostname(hostname)
        , m_prefilledUsername(prefilledUsername)
    {
        connect(this, SIGNAL(accepted(QString, QString)), SLOT(dismiss()));
        connect(this, SIGNAL(rejected()), SLOT(dismiss()));
    }

    QString hostname() const { return m_hostname; }
    QString prefilledUsername() const { return m_prefilledUsername; }

public Q_SLOTS:
    void accept(const QString& username, const QString& password) { emit accepted(username, password); }
    void reject() { emit rejected(); }

Q_SIGNALS:
    void accepted(const QString& username, const QString& password);
    void rejected();

private:
    QString m_hostname;
    QString m_prefilledUsername;
};

class HttpAuthenticationDialogContextObject : public BaseAuthenticationContextObject {
    Q_OBJECT
    Q_PROPERTY(QString realm READ realm CONSTANT)

public:
    HttpAuthenticationDialogContextObject(const QString& hostname, const QString& realm, const QString& prefilledUsername)
        : BaseAuthenticationContextObject(hostname, prefilledUsername)
        , m_realm(realm)
    {
    }

    QString realm() const { return m_realm; }

private:
    QString m_realm;
};

class ProxyAuthenticationDialogContextObject : public BaseAuthenticationContextObject {
    Q_OBJECT
    Q_PROPERTY(quint16 port READ port CONSTANT)

public:
    ProxyAuthenticationDialogContextObject(const QString& hostname, quint16 port, const QString& prefilledUsername)
        : BaseAuthenticationContextObject(hostname, prefilledUsername)
        , m_port(port)
    {
    }

    quint16 port() const { return m_port; }

private:
    quint16 m_port;
};

class CertificateVerificationDialogContextObject : public DialogContextBase {
    Q_OBJECT
    Q_PROPERTY(QString hostname READ hostname CONSTANT)

public:
    CertificateVerificationDialogContextObject(const QString& hostname)
        : DialogContextBase()
        , m_hostname(hostname)
    {
        connect(this, SIGNAL(accepted()), SLOT(dismiss()));
        connect(this, SIGNAL(rejected()), SLOT(dismiss()));
    }

    QString hostname() const { return m_hostname; }

public Q_SLOTS:
    void accept() { emit accepted(); }
    void reject() { emit rejected(); }

Q_SIGNALS:
    void accepted();
    void rejected();

private:
    QString m_hostname;
};

class FilePickerContextObject : public DialogContextBase {
    Q_OBJECT
    Q_PROPERTY(QStringList fileList READ fileList CONSTANT)
    Q_PROPERTY(bool allowMultipleFiles READ allowMultipleFiles CONSTANT)

public:
    FilePickerContextObject(const QStringList& selectedFiles, bool allowMultiple)
        : DialogContextBase()
        , m_allowMultiple(allowMultiple)
        , m_fileList(selectedFiles)
    {
        connect(this, SIGNAL(fileSelected(QStringList)), SLOT(dismiss()));
        connect(this, SIGNAL(rejected()), SLOT(dismiss()));
    }

    QStringList fileList() const { return m_fileList; }
    bool allowMultipleFiles() const { return m_allowMultiple;}

public Q_SLOTS:
    void reject() { emit rejected();}
    void accept(const QVariant& path)
    {
        QStringList filesPath = path.toStringList();

        if (filesPath.isEmpty()) {
            emit rejected();
            return;
        }

        // For single file upload, send only the first element if there are more than one file paths
        if (!m_allowMultiple && filesPath.count() > 1)
            filesPath = QStringList(filesPath.at(0));
        emit fileSelected(filesPath);
    }

Q_SIGNALS:
    void rejected();
    void fileSelected(const QStringList&);

private:
    bool m_allowMultiple;
    QStringList m_fileList;
};

class DatabaseQuotaDialogContextObject : public DialogContextBase {
    Q_OBJECT
    Q_PROPERTY(QString databaseName READ databaseName CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(quint64 currentQuota READ currentQuota CONSTANT)
    Q_PROPERTY(quint64 currentOriginUsage READ currentOriginUsage CONSTANT)
    Q_PROPERTY(quint64 currentDatabaseUsage READ currentDatabaseUsage CONSTANT)
    Q_PROPERTY(quint64 expectedUsage READ expectedUsage CONSTANT)
    Q_PROPERTY(QtWebSecurityOrigin* origin READ securityOrigin CONSTANT)

public:
    DatabaseQuotaDialogContextObject(const QString& databaseName, const QString& displayName, WKSecurityOriginRef securityOrigin, quint64 currentQuota, quint64 currentOriginUsage, quint64 currentDatabaseUsage, quint64 expectedUsage)
        : DialogContextBase()
        , m_databaseName(databaseName)
        , m_displayName(displayName)
        , m_currentQuota(currentQuota)
        , m_currentOriginUsage(currentOriginUsage)
        , m_currentDatabaseUsage(currentDatabaseUsage)
        , m_expectedUsage(expectedUsage)
    {
        WKRetainPtr<WKStringRef> scheme = adoptWK(WKSecurityOriginCopyProtocol(securityOrigin));
        WKRetainPtr<WKStringRef> host = adoptWK(WKSecurityOriginCopyHost(securityOrigin));

        m_securityOrigin.setScheme(WKStringCopyQString(scheme.get()));
        m_securityOrigin.setHost(WKStringCopyQString(host.get()));
        m_securityOrigin.setPort(static_cast<int>(WKSecurityOriginGetPort(securityOrigin)));

        connect(this, SIGNAL(accepted(quint64)), SLOT(dismiss()));
        connect(this, SIGNAL(rejected()), SLOT(dismiss()));
    }

    QString databaseName() const { return m_databaseName; }
    QString displayName() const { return m_displayName; }
    quint64 currentQuota() const { return m_currentQuota; }
    quint64 currentOriginUsage() const { return m_currentOriginUsage; }
    quint64 currentDatabaseUsage() const { return m_currentDatabaseUsage; }
    quint64 expectedUsage() const { return m_expectedUsage; }
    QtWebSecurityOrigin* securityOrigin() { return &m_securityOrigin; }

public Q_SLOTS:
    void accept(quint64 size) { emit accepted(size); }
    void reject() { emit rejected(); }

Q_SIGNALS:
    void accepted(quint64 size);
    void rejected();

private:
    QString m_databaseName;
    QString m_displayName;
    quint64 m_currentQuota;
    quint64 m_currentOriginUsage;
    quint64 m_currentDatabaseUsage;
    quint64 m_expectedUsage;
    QtWebSecurityOrigin m_securityOrigin;
};

void QtDialogRunner::run()
{
    DialogContextBase* context = static_cast<DialogContextBase*>(m_dialogContext->contextObject());

    // We may have already been dismissed as part of Component.onCompleted()
    if (context->m_dismissed)
        return;

    connect(context, SIGNAL(dismissed()), SLOT(quit()));
    exec(); // Spin the event loop
}

bool QtDialogRunner::initForAlert(const QString& message)
{
    QQmlComponent* component = m_webView->experimental()->alertDialog();
    if (!component)
        return false;

    DialogContextObject* contextObject = new DialogContextObject(message);

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForConfirm(const QString& message)
{
    QQmlComponent* component = m_webView->experimental()->confirmDialog();
    if (!component)
        return false;

    DialogContextObject* contextObject = new DialogContextObject(message);
    connect(contextObject, SIGNAL(accepted(QString)), SLOT(onAccepted()));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForPrompt(const QString& message, const QString& defaultValue)
{
    QQmlComponent* component = m_webView->experimental()->promptDialog();
    if (!component)
        return false;

    DialogContextObject* contextObject = new DialogContextObject(message, defaultValue);
    connect(contextObject, SIGNAL(accepted(QString)), SLOT(onAccepted(QString)));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForAuthentication(const QString& hostname, const QString& realm, const QString& prefilledUsername)
{
    QQmlComponent* component = m_webView->experimental()->authenticationDialog();
    if (!component)
        return false;

    HttpAuthenticationDialogContextObject* contextObject = new HttpAuthenticationDialogContextObject(hostname, realm, prefilledUsername);
    connect(contextObject, SIGNAL(accepted(QString, QString)), SLOT(onAuthenticationAccepted(QString, QString)));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForProxyAuthentication(const QString& hostname, uint16_t port, const QString& prefilledUsername)
{
    QQmlComponent* component = m_webView->experimental()->proxyAuthenticationDialog();
    if (!component)
        return false;

    ProxyAuthenticationDialogContextObject* contextObject = new ProxyAuthenticationDialogContextObject(hostname, port, prefilledUsername);
    connect(contextObject, SIGNAL(accepted(QString, QString)), SLOT(onAuthenticationAccepted(QString, QString)));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForCertificateVerification(const QString& hostname)
{
    QQmlComponent* component = m_webView->experimental()->certificateVerificationDialog();
    if (!component)
        return false;

    CertificateVerificationDialogContextObject* contextObject = new CertificateVerificationDialogContextObject(hostname);
    connect(contextObject, SIGNAL(accepted()), SLOT(onAccepted()));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForFilePicker(const QStringList& selectedFiles, bool allowMultiple)
{
    QQmlComponent* component = m_webView->experimental()->filePicker();
    if (!component)
        return false;

    FilePickerContextObject* contextObject = new FilePickerContextObject(selectedFiles, allowMultiple);
    connect(contextObject, SIGNAL(fileSelected(QStringList)), SLOT(onFileSelected(QStringList)));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForDatabaseQuotaDialog(const QString& databaseName, const QString& displayName, WKSecurityOriginRef securityOrigin, quint64 currentQuota, quint64 currentOriginUsage, quint64 currentDatabaseUsage, quint64 expectedUsage)
{
    QQmlComponent* component = m_webView->experimental()->databaseQuotaDialog();
    if (!component)
        return false;

    DatabaseQuotaDialogContextObject* contextObject = new DatabaseQuotaDialogContextObject(databaseName, displayName, securityOrigin, currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage);
    connect(contextObject, SIGNAL(accepted(quint64)), SLOT(onDatabaseQuotaAccepted(quint64)));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::createDialog(QQmlComponent* component, QObject* contextObject)
{
    QQmlContext* baseContext = component->creationContext();
    if (!baseContext)
        baseContext = QQmlEngine::contextForObject(m_webView);
    m_dialogContext = adoptPtr(new QQmlContext(baseContext));

    // This makes both "message" and "model.message" work for the dialog,
    // just like QtQuick's ListView delegates.
    contextObject->setParent(m_dialogContext.get());
    m_dialogContext->setContextProperty(QLatin1String("model"), contextObject);
    m_dialogContext->setContextObject(contextObject);

    QObject* object = component->beginCreate(m_dialogContext.get());
    if (!object) {
        m_dialogContext.clear();
        return false;
    }

    m_dialog = adoptPtr(qobject_cast<QQuickItem*>(object));
    if (!m_dialog) {
        m_dialogContext.clear();
        m_dialog.clear();
        return false;
    }

    QQuickWebViewPrivate::get(m_webView)->addAttachedPropertyTo(m_dialog.get());
    m_dialog->setParentItem(m_webView);

    // Only fully create the component once we've set both a parent
    // and the needed context and attached properties, so that dialogs
    // can do useful stuff in their Component.onCompleted() method.
    component->completeCreate();

    // FIXME: As part of completeCreate, the bindings of the item will be
    // evaluated, but for some reason doing mapToItem/mapFromItem in a
    // binding will not work as expected, even if we at binding evaluation
    // time have the parent and all the way up to the root QML item.
    // As a workaround you can set whichever property you need in
    // Component.onCompleted, even to a binding using Qt.bind().

    return true;
}

void QtDialogRunner::onAccepted(const QString& result)
{
    m_wasAccepted = true;
    m_result = result;
}

void QtDialogRunner::onAuthenticationAccepted(const QString& username, const QString& password)
{
    m_username = username;
    m_password = password;
}

void QtDialogRunner::onFileSelected(const QStringList& filePaths)
{
    m_wasAccepted = true;
    m_filepaths = filePaths;
}

void QtDialogRunner::onDatabaseQuotaAccepted(quint64 quota)
{
    m_wasAccepted = true;
    m_databaseQuota = quota;
}

} // namespace WebKit

#include "QtDialogRunner.moc"
#include "moc_QtDialogRunner.cpp"

