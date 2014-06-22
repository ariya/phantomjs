/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 University of Szeged. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TestRunner.h"

#include "ActivateFonts.h"
#include "InjectedBundle.h"
#include "QtTestSupport.h"
#include <QCoreApplication>
#include <QDir>
#include <QFontDatabase>
#include <QObject>
#include <QtCore/qglobal.h>
#include <qwebsettings.h>

namespace WTR {

class WatchdogTimerHelper : public QObject {
    Q_OBJECT

public:
    static WatchdogTimerHelper* instance()
    {
        static WatchdogTimerHelper* theInstance = new WatchdogTimerHelper;
        return theInstance;
    }

public Q_SLOTS:
    void timerFired()
    {
        InjectedBundle::shared().testRunner()->waitToDumpWatchdogTimerFired();
    }

private:
    WatchdogTimerHelper() {}
};

void TestRunner::platformInitialize()
{
    WebKit::QtTestSupport::clearMemoryCaches();
    activateFonts();

    QObject::connect(&m_waitToDumpWatchdogTimer, SIGNAL(timeout()), WatchdogTimerHelper::instance(), SLOT(timerFired()));
}

void TestRunner::invalidateWaitToDumpWatchdogTimer()
{
    m_waitToDumpWatchdogTimer.stop();
}

void TestRunner::initializeWaitToDumpWatchdogTimerIfNeeded()
{
    int timerInterval;
    if (qgetenv("QT_WEBKIT2_DEBUG") == "1")
        return;

    if (m_waitToDumpWatchdogTimer.isActive())
        return;
    if (m_timeout > 0)
        timerInterval = m_timeout;
    else
        timerInterval = waitToDumpWatchdogTimerInterval * 1000;

    m_waitToDumpWatchdogTimer.start(timerInterval);
}

JSRetainPtr<JSStringRef> TestRunner::pathToLocalResource(JSStringRef url)
{
    QString localTmpUrl(QStringLiteral("file:///tmp/LayoutTests"));
    QString givenUrl(reinterpret_cast<const QChar*>(JSStringGetCharactersPtr(url)), JSStringGetLength(url));

    // Translate a request for /tmp/LayoutTests to the repository LayoutTests directory.
    // Do not rely on a symlink to be created via the test runner, which will not work on Windows.
    if (givenUrl.startsWith(localTmpUrl)) {
        // DumpRenderTree lives in WebKit/WebKitBuild/<build_mode>/bin.
        // Translate from WebKit/WebKitBuild/Release/bin => WebKit/LayoutTests.
        QFileInfo layoutTestsRoot(QCoreApplication::applicationDirPath() + QStringLiteral("/../../../LayoutTests/"));
        if (layoutTestsRoot.exists()) {
            QString path = QStringLiteral("file://") + layoutTestsRoot.absolutePath() + givenUrl.mid(localTmpUrl.length());
            return JSStringCreateWithCharacters(reinterpret_cast<const JSChar*>(path.constData()), path.length());
        }
    }
    return url;
}

JSRetainPtr<JSStringRef> TestRunner::platformName()
{
    JSRetainPtr<JSStringRef> platformName(Adopt, JSStringCreateWithUTF8CString("qt"));
    return platformName;
}

} // namespace WTR

#include "TestRunnerQt.moc"
