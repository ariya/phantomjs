/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
  Copyright (C) 2017 Vitaly Slobodin <vitaliy.slobodin@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "custompage.h"

CustomPage::CustomPage(WebPage* parent) : QWebPage(parent), m_webPage(parent)
{
    m_userAgent = QWebPage::userAgentForUrl(QUrl());
    setForwardUnsupportedContent(true);
    connect(this, &QWebPage::consoleMessageReceived, this, &CustomPage::onConsoleMessageReceived);
}

bool CustomPage::extension(Extension extension, const ExtensionOption* option, ExtensionReturn* output)
{
    Q_UNUSED(option);
    if (extension == ChooseMultipleFilesExtension) {
        static_cast<ChooseMultipleFilesExtensionReturn*>(output)->fileNames = m_uploadFiles;
        return true;
    }
    return false;
}

void CustomPage::setCookieJar(CookieJar* cookieJar)
{
    m_cookieJar = cookieJar;
}

bool CustomPage::shouldInterruptJavaScript()
{
    m_webPage->javascriptInterrupt();
    if (m_webPage->m_shouldInterruptJs) {
        // reset our flag
        m_webPage->m_shouldInterruptJs = false;
        return true;
    }
    return false;
}

bool CustomPage::supportsExtension(Extension extension) const
{
    return extension == ChooseMultipleFilesExtension;
}

QString CustomPage::chooseFile(QWebFrame* originatingFrame, const QString& oldFile)
{
    Q_UNUSED(originatingFrame);

    // Check if User set a file via File Picker
    QString chosenFile = m_webPage->filePicker(oldFile);
    if (chosenFile == QString::null && m_uploadFiles.count() > 0) {
        // Check if instead User set a file via uploadFile API
        chosenFile = m_uploadFiles.first();
    }

    // Return the value coming from the "filePicker" callback, IFF not null.
    qDebug() << "CustomPage - file chosen for upload:" << chosenFile;
    return chosenFile;
}

void CustomPage::javaScriptAlert(QWebFrame* originatingFrame, const QString& msg)
{
    Q_UNUSED(originatingFrame);
    emit m_webPage->javaScriptAlertSent(msg);
}

bool CustomPage::javaScriptConfirm(QWebFrame* originatingFrame, const QString& msg)
{
    Q_UNUSED(originatingFrame);
    return m_webPage->javaScriptConfirm(msg);
}

bool CustomPage::javaScriptPrompt(QWebFrame* originatingFrame, const QString& msg, const QString& defaultValue, QString* result)
{
    Q_UNUSED(originatingFrame);
    return m_webPage->javaScriptPrompt(msg, defaultValue, result);
}

void CustomPage::onConsoleMessageReceived(MessageSource source, MessageLevel level, const QString& message, int lineNumber, const QString& sourceID)
{
    Q_UNUSED(source);
    qDebug() << message << lineNumber << sourceID;

    if (level == ErrorMessageLevel) {
        emit m_webPage->javaScriptErrorSent(message, lineNumber, sourceID, QString());
    } else {
        emit m_webPage->javaScriptConsoleMessageSent(message, lineNumber, sourceID);
    }
}

QString CustomPage::userAgentForUrl(const QUrl& url) const
{
    Q_UNUSED(url);
    return m_userAgent;
}

bool CustomPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type)
{
    bool isMainFrame = frame == m_webPage->m_mainFrame;

    QString navigationType = "Undefined";
    switch (type) {
    case NavigationTypeLinkClicked:
        navigationType = "LinkClicked";
        break;
    case NavigationTypeFormSubmitted:
        navigationType = "FormSubmitted";
        break;
    case NavigationTypeBackOrForward:
        navigationType = "BackOrForward";
        break;
    case NavigationTypeReload:
        navigationType = "Reload";
        break;
    case NavigationTypeFormResubmitted:
        navigationType = "FormResubmitted";
        break;
    case NavigationTypeOther:
        navigationType = "Other";
        break;
    }
    bool isNavigationLocked = m_webPage->navigationLocked();

    emit m_webPage->navigationRequested(
        request.url().toEncoded(),       //< Requested URL
        navigationType,                  //< Navigation Type
        !isNavigationLocked,             //< Will navigate (not locked)?
        isMainFrame);                    //< Is main frame?

    return !isNavigationLocked;
}

QWebPage* CustomPage::createWindow(WebWindowType type)
{
    Q_UNUSED(type);
    WebPage* newPage;

    // Create a new "raw" WebPage object
    if (m_webPage->ownsPages()) {
        newPage = new WebPage(m_webPage);
    } else {
        newPage = new WebPage(Phantom::instance());
        Phantom::instance()->appendPage(newPage);
    }
    newPage->setCookieJar(m_cookieJar);

    // Apply default settings
    newPage->applySettings(Phantom::instance()->defaultPageSettings());

    // Signal JS shim to catch, decorate and store this new child page
    emit m_webPage->rawPageCreated(newPage);

    // Return the new QWebPage to the QWebKit backend
    return newPage->m_customWebPage;
}

void CustomPage::setUserAgent(QString userAgent)
{
    m_userAgent = userAgent;
}

QString CustomPage::userAgent() const
{
    return m_userAgent;
}

void CustomPage::clearUploadFiles()
{
    m_uploadFiles.clear();
}

void CustomPage::addFileToUpload(QString filename)
{
    m_uploadFiles.append(filename);
}
