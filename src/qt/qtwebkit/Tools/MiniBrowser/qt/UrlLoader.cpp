/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 University of Szeged
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

#include "UrlLoader.h"
#include "private/qquickwebview_p.h"
#include "private/qwebloadrequest_p.h"

#include <QDebug>
#include <QFile>

UrlLoader::UrlLoader(BrowserWindow* browserWindow, const QString& inputFileName, int timeoutSeconds, int extraTimeSeconds)
    : m_browserWindow(browserWindow)
    , m_stdOut(stdout)
    , m_loaded(0)
    , m_numFramesLoading(0)
{
    m_checkIfFinishedTimer.setInterval(200);
    m_checkIfFinishedTimer.setSingleShot(true);
    connect(&m_checkIfFinishedTimer, SIGNAL(timeout()), this, SLOT(checkIfFinished()));
    // loadStarted and loadFinished on QWebPage is emitted for each frame/sub-frame
    connect(m_browserWindow->webView(), SIGNAL(loadingChanged(QWebLoadRequest*)), this, SLOT(loadingChanged(QWebLoadRequest*)));
    connect(this, SIGNAL(loadStarted()), this, SLOT(frameLoadStarted()));
    connect(this, SIGNAL(loadFinished()), this, SLOT(frameLoadFinished()));

    if (timeoutSeconds) {
        m_timeoutTimer.setInterval(timeoutSeconds * 1000);
        m_timeoutTimer.setSingleShot(true);
        connect(this, SIGNAL(loadStarted()), &m_timeoutTimer, SLOT(start()));
        connect(&m_timeoutTimer, SIGNAL(timeout()), this, SLOT(loadNext()));
    }
    if (extraTimeSeconds) {
        m_extraTimeTimer.setInterval(extraTimeSeconds * 1000);
        m_extraTimeTimer.setSingleShot(true);
        connect(this, SIGNAL(pageLoadFinished()), &m_extraTimeTimer, SLOT(start()));
        connect(&m_extraTimeTimer, SIGNAL(timeout()), this, SLOT(loadNext()));
    } else
        connect(this, SIGNAL(pageLoadFinished()), this, SLOT(loadNext()));
    loadUrlList(inputFileName);
}

void UrlLoader::loadNext()
{
    m_timeoutTimer.stop();
    m_extraTimeTimer.stop();
    m_checkIfFinishedTimer.stop();
    m_numFramesLoading = 0;
    QString qstr;
    if (getUrl(qstr)) {
        QUrl url(qstr, QUrl::StrictMode);
        if (url.isValid()) {
            m_stdOut << "Loading " << qstr << " ......" << ++m_loaded << endl;
            m_browserWindow->load(url.toString());
        } else
            loadNext();
    } else
        disconnect(m_browserWindow, 0, this, 0);
}

void UrlLoader::checkIfFinished()
{
    if (!m_numFramesLoading)
        emit pageLoadFinished();
}

void UrlLoader::frameLoadStarted()
{
    ++m_numFramesLoading;
    m_checkIfFinishedTimer.stop();
}

void UrlLoader::frameLoadFinished()
{
    Q_ASSERT(m_numFramesLoading > 0);
    --m_numFramesLoading;
    // Once our frame has finished loading, wait a moment to call loadNext for cases
    // where a sub-frame starts loading or another frame is loaded through JavaScript.
    m_checkIfFinishedTimer.start();
}

void UrlLoader::loadUrlList(const QString& inputFileName)
{
    QFile inputFile(inputFileName);
    if (inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&inputFile);
        QString line;
        while (true) {
            line = stream.readLine();
            if (line.isNull())
                break;
            m_urls.append(line);
        }
    } else {
        qDebug() << "Can't open list file";
        exit(0);
    }
    m_index = 0;
    inputFile.close();
}

bool UrlLoader::getUrl(QString& qstr)
{
    if (m_index == m_urls.size())
        return false;

    qstr = m_urls[m_index++];
    return true;
}

void UrlLoader::loadingChanged(QWebLoadRequest* loadRequest)
{
    switch (loadRequest->status()) {
    case QQuickWebView::LoadStartedStatus:
        emit loadStarted();
        break;
    case QQuickWebView::LoadStoppedStatus:
    case QQuickWebView::LoadSucceededStatus:
    case QQuickWebView::LoadFailedStatus:
    default:
        emit loadFinished();
        break;
    }
}
