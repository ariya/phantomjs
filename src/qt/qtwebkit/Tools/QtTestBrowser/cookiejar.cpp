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

#include "config.h"

#include "cookiejar.h"

#include <QStandardPaths>
#include <QDir>
#include <QTextStream>

TestBrowserCookieJar::TestBrowserCookieJar(QObject* parent)
    : QNetworkCookieJar(parent)
    , m_storageEnabled(false)
{
    // We use a timer for the real disk write to avoid multiple IO
    // syscalls in sequence (when loading pages which set multiple cookies).
    m_timer.setInterval(10000);
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(saveToDisk()));

#ifndef QT_NO_DESKTOPSERVICES
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    QString path = QDir::homePath() + "/.QtTestBrowser";
#endif

    QDir().mkpath(path);
    m_file.setFileName(path + "/cookieJar");
}

TestBrowserCookieJar::~TestBrowserCookieJar()
{
    if (m_storageEnabled) {
        extractRawCookies();
        saveToDisk();
    }
}

bool TestBrowserCookieJar::setCookiesFromUrl(const QList<QNetworkCookie>& cookieList, const QUrl& url)
{
    bool status = QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
    if (status && m_storageEnabled)
        scheduleSaveToDisk();
    return status;
}

void TestBrowserCookieJar::setDiskStorageEnabled(bool enabled)
{
    m_storageEnabled = enabled;

    if (enabled && allCookies().isEmpty())
        loadFromDisk();

    // When disabling, save current cookies.
    if (!enabled && !allCookies().isEmpty())
        scheduleSaveToDisk();
}

void TestBrowserCookieJar::scheduleSaveToDisk()
{
    // We extract the raw cookies here because the user may
    // enable/disable/clear cookies while the timer is running.
    extractRawCookies();
    m_timer.start();
}

void TestBrowserCookieJar::extractRawCookies()
{
    QList<QNetworkCookie> cookies = allCookies();
    m_rawCookies.clear();

    foreach (const QNetworkCookie &cookie, cookies) {
        if (!cookie.isSessionCookie())
            m_rawCookies.append(cookie.toRawForm());
    }
}

void TestBrowserCookieJar::saveToDisk()
{
    m_timer.stop();

    if (m_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&m_file);
        foreach (const QByteArray &cookie, m_rawCookies)
            out << cookie + "\n";
        m_file.close();
    } else
        qWarning("IO error handling cookiejar file");
}

void TestBrowserCookieJar::loadFromDisk()
{
    if (!m_file.exists())
        return;

    QList<QNetworkCookie> cookies;

    if (m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&m_file);
        while (!in.atEnd())
            cookies.append(QNetworkCookie::parseCookies(in.readLine().toUtf8()));
        m_file.close();
    } else
        qWarning("IO error handling cookiejar file");

    setAllCookies(cookies);
}

void TestBrowserCookieJar::reset()
{
    setAllCookies(QList<QNetworkCookie>());
    if (m_storageEnabled)
        scheduleSaveToDisk();
}
