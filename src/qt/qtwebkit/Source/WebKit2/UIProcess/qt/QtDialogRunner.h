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

#ifndef QtDialogRunner_h
#define QtDialogRunner_h

#include "WKSecurityOrigin.h"
#include "qquickwebview_p.h"
#include <QtCore/QEventLoop>
#include <QtCore/QStringList>
#include <wtf/OwnPtr.h>

QT_BEGIN_NAMESPACE
class QQmlComponent;
class QQmlContext;
class QQuickItem;
QT_END_NAMESPACE

namespace WebKit {

class QtDialogRunner : public QEventLoop {
    Q_OBJECT

public:
    QtDialogRunner(QQuickWebView*);
    virtual ~QtDialogRunner();

    bool initForAlert(const QString& message);
    bool initForConfirm(const QString& message);
    bool initForPrompt(const QString& message, const QString& defaultValue);
    bool initForAuthentication(const QString& hostname, const QString& realm, const QString& prefilledUsername);
    bool initForCertificateVerification(const QString& hostname);
    bool initForProxyAuthentication(const QString& hostname, uint16_t port, const QString& prefilledUsername);
    bool initForFilePicker(const QStringList& selectedFiles, bool allowMultiple);
    bool initForDatabaseQuotaDialog(const QString& databaseName, const QString& displayName, WKSecurityOriginRef, quint64 currentQuota, quint64 currentOriginUsage, quint64 currentDatabaseUsage, quint64 expectedUsage);

    void run();

    QQuickItem* dialog() const { return m_dialog.get(); }

    bool wasAccepted() const { return m_wasAccepted; }
    QString result() const { return m_result; }

    QString username() const { return m_username; }
    QString password() const { return m_password; }

    quint64 databaseQuota() const { return m_databaseQuota; }

    QStringList filePaths() const { return m_filepaths; }

public Q_SLOTS:
    void onAccepted(const QString& result = QString());
    void onAuthenticationAccepted(const QString& username, const QString& password);
    void onFileSelected(const QStringList& filePaths);
    void onDatabaseQuotaAccepted(quint64 quota);

private:
    bool createDialog(QQmlComponent*, QObject* contextObject);

    QQuickWebView* m_webView;
    OwnPtr<QQmlContext> m_dialogContext;
    OwnPtr<QQuickItem> m_dialog;
    QString m_result;
    bool m_wasAccepted;

    QString m_username;
    QString m_password;
    QStringList m_filepaths;
    quint64 m_databaseQuota;
};

} // namespace WebKit

#endif // QtDialogRunner_h
