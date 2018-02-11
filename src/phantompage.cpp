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

#include <QFile>
#include <QWebFrame>

#include "phantompage.h"

PhantomPage::PhantomPage(QObject* parent)
    : QObject(parent)
{
    setObjectName("PhantomPage");

    m_webPage = new QWebPage(this);
    m_webPage->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
    m_webPage->settings()->setAttribute(QWebSettings::WebSecurityEnabled, false);
    m_webPage->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);

    connect(m_webPage, &QWebPage::consoleMessageReceived, this, &PhantomPage::onConsoleMessageReceived);
}

PhantomPage::~PhantomPage()
{
    qDebug() << __PRETTY_FUNCTION__;

    m_webPage->deleteLater();
}

void PhantomPage::bootstrap()
{
    qDebug() << __PRETTY_FUNCTION__;

    // Add 'phantom' object to the global scope
    m_webPage->mainFrame()->addToJavaScriptWindowObject("phantom", parent());

    // Bootstrap the PhantomJS scope
    executeFromFile(":/bootstrap.js");
}

QVariant PhantomPage::execute(const QString& source)
{
    qDebug() << __PRETTY_FUNCTION__;
    return m_webPage->mainFrame()->evaluateJavaScript(source);
}

QVariant PhantomPage::executeFromFile(const QString& filePath)
{
    qDebug() << __PRETTY_FUNCTION__ << filePath;

    QFile file(filePath);

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return QVariant::Invalid;
    }

    QTextStream in(&file);

    // by default we read script in UTF-8
    in.setCodec("UTF-8");
    return execute(in.readAll());
}

QWebFrame* PhantomPage::mainFrame() const
{
    Q_ASSERT(m_webPage);

    return m_webPage->mainFrame();
}

void PhantomPage::showInspector(qint16 port)
{
    if (!m_webInspector)
        m_webInspector = new QWebInspector();

    //m_webInspector.setPort(port);
}

// private

void PhantomPage::onConsoleMessageReceived(
        QWebPage::MessageSource source,
        QWebPage::MessageLevel level,
        const QString& message,
        int lineNumber,
        const QString& sourceID)
{
    Q_UNUSED(source);
    Q_UNUSED(level);

    qDebug() << message << lineNumber << sourceID;

    if (level == QWebPage::ErrorMessageLevel) {
        emit javaScriptErrorSent(message);
    } else {
        emit consoleMessageReceived(message, lineNumber, sourceID);
    }
}

void PhantomPage::onLoadFinished(bool status)
{
    qDebug() << __PRETTY_FUNCTION__;

    emit initialized(status);
}
