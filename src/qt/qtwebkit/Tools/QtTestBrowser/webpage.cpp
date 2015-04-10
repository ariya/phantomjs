/*
 * Copyright (C) 2009-2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "webpage.h"

#include "launcherwindow.h"

#include <QAction>
#include <QApplication>
#include <QAuthenticator>
#ifndef QT_NO_DESKTOPSERVICES
#include <QDesktopServices>
#endif
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#ifndef QT_NO_LINEEDIT
#include <QLineEdit>
#endif
#include <QProgressBar>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkProxy>

WebPage::WebPage(QObject* parent)
    : QWebPage(parent)
    , m_userAgent()
    , m_interruptingJavaScriptEnabled(false)
{
    applyProxy();

    connect(networkAccessManager(), SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
            this, SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));
    connect(this, SIGNAL(featurePermissionRequested(QWebFrame*, QWebPage::Feature)), this, SLOT(requestPermission(QWebFrame*, QWebPage::Feature)));
    connect(this, SIGNAL(featurePermissionRequestCanceled(QWebFrame*, QWebPage::Feature)), this, SLOT(featurePermissionRequestCanceled(QWebFrame*, QWebPage::Feature)));
}

void WebPage::applyProxy()
{
    QUrl proxyUrl(qgetenv("http_proxy"));

    if (proxyUrl.isValid() && !proxyUrl.host().isEmpty()) {
        int proxyPort = (proxyUrl.port() > 0) ? proxyUrl.port() : 8080;
        networkAccessManager()->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, proxyUrl.host(), proxyPort));
    }
}

bool WebPage::supportsExtension(QWebPage::Extension extension) const
{
    if (extension == QWebPage::ErrorPageExtension)
        return true;
    return false;
}

bool WebPage::extension(Extension extension, const ExtensionOption* option, ExtensionReturn* output)
{
    const QWebPage::ErrorPageExtensionOption* info = static_cast<const QWebPage::ErrorPageExtensionOption*>(option);
    QWebPage::ErrorPageExtensionReturn* errorPage = static_cast<QWebPage::ErrorPageExtensionReturn*>(output);

    errorPage->content = QString("<html><head><title>Failed loading page</title></head><body>%1</body></html>")
        .arg(info->errorString).toUtf8();

    return true;
}

bool WebPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type)
{
    QObject* view = parent();

    QVariant value = view->property("keyboardModifiers");

    if (!value.isNull()) {
        Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers(value.toInt());

        if (modifiers & Qt::ShiftModifier) {
            QWebPage* page = createWindow(QWebPage::WebBrowserWindow);
            page->mainFrame()->load(request);
            return false;
        }

        if (modifiers & Qt::AltModifier) {
            openUrlInDefaultBrowser(request.url());
            return false;
        }
    }

    return QWebPage::acceptNavigationRequest(frame, request, type);
}

void WebPage::openUrlInDefaultBrowser(const QUrl& url)
{
#ifndef QT_NO_DESKTOPSERVICES
    if (QAction* action = qobject_cast<QAction*>(sender()))
        QDesktopServices::openUrl(action->data().toUrl());
    else
        QDesktopServices::openUrl(url);
#endif
}

QString WebPage::userAgentForUrl(const QUrl& url) const
{
    if (!m_userAgent.isEmpty())
        return m_userAgent;
    return QWebPage::userAgentForUrl(url);
}

bool WebPage::shouldInterruptJavaScript()
{
    if (!m_interruptingJavaScriptEnabled)
        return false;
    return QWebPage::shouldInterruptJavaScript();
}

void WebPage::authenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator)
{
    QDialog* dialog = new QDialog(QApplication::activeWindow());
    dialog->setWindowTitle("HTTP Authentication");

    QGridLayout* layout = new QGridLayout(dialog);
    dialog->setLayout(layout);

    QLabel* messageLabel = new QLabel(dialog);
    messageLabel->setWordWrap(true);
    QString messageStr = QString("Enter with username and password for: %1");
    messageLabel->setText(messageStr.arg(reply->url().toString()));
    layout->addWidget(messageLabel, 0, 1);

#ifndef QT_NO_LINEEDIT
    QLabel* userLabel = new QLabel("Username:", dialog);
    layout->addWidget(userLabel, 1, 0);
    QLineEdit* userInput = new QLineEdit(dialog);
    layout->addWidget(userInput, 1, 1);

    QLabel* passLabel = new QLabel("Password:", dialog);
    layout->addWidget(passLabel, 2, 0);
    QLineEdit* passInput = new QLineEdit(dialog);
    passInput->setEchoMode(QLineEdit::Password);
    layout->addWidget(passInput, 2, 1);
#endif

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
            | QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
    connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));
    layout->addWidget(buttonBox, 3, 1);

    if (dialog->exec() == QDialog::Accepted) {
#ifndef QT_NO_LINEEDIT
        authenticator->setUser(userInput->text());
        authenticator->setPassword(passInput->text());
#endif
    }

    delete dialog;
}

void WebPage::requestPermission(QWebFrame* frame, QWebPage::Feature feature)
{
    setFeaturePermission(frame, feature, PermissionGrantedByUser);
}

void WebPage::featurePermissionRequestCanceled(QWebFrame*, QWebPage::Feature)
{
}

QWebPage* WebPage::createWindow(QWebPage::WebWindowType type)
{
    LauncherWindow* mw = new LauncherWindow;
    if (type == WebModalDialog)
        mw->setWindowModality(Qt::ApplicationModal);
    mw->show();
    return mw->page();
}

QObject* WebPage::createPlugin(const QString &classId, const QUrl&, const QStringList&, const QStringList&)
{
    if (classId == "alien_QLabel") {
        QLabel* l = new QLabel;
        l->winId();
        return l;
    }

    if (classId == QLatin1String("QProgressBar"))
        return new QProgressBar(view());
    if (classId == QLatin1String("QLabel"))
        return new QLabel(view());
    return 0;
}

