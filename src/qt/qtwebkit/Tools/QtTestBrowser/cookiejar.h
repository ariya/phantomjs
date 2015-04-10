/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef cookiejar_h
#define cookiejar_h

#include <QFile>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QTimer>

class TestBrowserCookieJar : public QNetworkCookieJar {
    Q_OBJECT

public:
    TestBrowserCookieJar(QObject* parent = 0);
    virtual ~TestBrowserCookieJar();

    virtual bool setCookiesFromUrl(const QList<QNetworkCookie>&, const QUrl&);

    void setDiskStorageEnabled(bool);

public Q_SLOTS:
    void scheduleSaveToDisk();
    void loadFromDisk();
    void reset();

private Q_SLOTS:
    void saveToDisk();

private:
    void extractRawCookies();

    QList<QByteArray> m_rawCookies;
    bool m_storageEnabled;
    QFile m_file;
    QTimer m_timer;
};

#endif
