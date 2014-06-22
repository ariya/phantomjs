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

#ifndef urlloader_h
#define urlloader_h

#include "qwebframe.h"

#include <QTextStream>
#include <QTimer>
#include <QVector>

class UrlLoader : public QObject {
    Q_OBJECT

public:
    UrlLoader(QWebFrame* frame, const QString& inputFileName, int timeoutSeconds, int extraTimeSeconds);

public Q_SLOTS:
    void loadNext();

private Q_SLOTS:
    void checkIfFinished();
    void frameLoadStarted();
    void frameLoadFinished();

Q_SIGNALS:
    void pageLoadFinished();

private:
    void loadUrlList(const QString& inputFileName);
    bool getUrl(QString& qstr);

private:
    QVector<QString> m_urls;
    int m_index;
    QWebFrame* m_frame;
    QTextStream m_stdOut;
    int m_loaded;
    QTimer m_timeoutTimer;
    QTimer m_extraTimeTimer;
    QTimer m_checkIfFinishedTimer;
    int m_numFramesLoading;
};

#endif
