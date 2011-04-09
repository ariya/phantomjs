/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>

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

#include <iostream>
#include <QApplication>

#include "webpage.h"

// public:
WebPage::WebPage(QObject *parent)
    : QWebPage(parent)
{
    m_userAgent = QWebPage::userAgentForUrl(QUrl());
    connect(this->currentFrame(), SIGNAL(urlChanged(QUrl)), this, SLOT(handleFrameUrlChanged(QUrl)));
    connect(this, SIGNAL(linkClicked(QUrl)), this, SLOT(handleLinkClicked(QUrl)));
}

// public slots:
bool WebPage::shouldInterruptJavaScript()
{
    QApplication::processEvents(QEventLoop::AllEvents, 42);
    return false;
}

// private slots:
void WebPage::handleFrameUrlChanged(const QUrl &url) {
    qDebug() << "URL Changed: " << qPrintable(url.toString());
}

void WebPage::handleLinkClicked(const QUrl &url) {
    qDebug() << "URL Clicked: " << qPrintable(url.toString());
}

// protected:
void WebPage::javaScriptAlert(QWebFrame *originatingFrame, const QString &msg)
{
    Q_UNUSED(originatingFrame);
    std::cout << "JavaScript alert: " << qPrintable(msg) << std::endl;
}

void WebPage::javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID)
{
    if (!sourceID.isEmpty())
        std::cout << qPrintable(sourceID) << ":" << lineNumber << " ";
    std::cout << qPrintable(message) << std::endl;
}

QString WebPage::userAgentForUrl(const QUrl &url) const
{
    Q_UNUSED(url);
    return m_userAgent;
}

QString WebPage::chooseFile(QWebFrame *parentFrame, const QString &suggestedFile)
{
    Q_UNUSED(parentFrame);
    Q_UNUSED(suggestedFile);
    if (m_allowedFiles.contains(m_nextFileTag))
        return m_allowedFiles.value(m_nextFileTag);
    return QString();
}
