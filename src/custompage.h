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

#include <QWebPage>
#include "phantom.h"
#include "webpage.h"

class CustomPage : public QWebPage
{
    Q_OBJECT

public: CustomPage(WebPage* parent = Q_NULLPTR);

    bool extension(Extension extension, const ExtensionOption* option, ExtensionReturn* output) Q_DECL_OVERRIDE;
    void setCookieJar(CookieJar* cookieJar);
    void setUserAgent(QString userAgent);
    QString userAgent() const;
    void addFileToUpload(QString filename);
    void clearUploadFiles();

public Q_SLOTS:
    bool shouldInterruptJavaScript() Q_DECL_OVERRIDE;

protected:
    bool supportsExtension(Extension extension) const Q_DECL_OVERRIDE;
    QString chooseFile(QWebFrame* originatingFrame, const QString& oldFile) Q_DECL_OVERRIDE;
    void javaScriptAlert(QWebFrame* originatingFrame, const QString& msg) Q_DECL_OVERRIDE;
    bool javaScriptConfirm(QWebFrame* originatingFrame, const QString& msg) Q_DECL_OVERRIDE;
    bool javaScriptPrompt(QWebFrame* originatingFrame, const QString& msg, const QString& defaultValue, QString* result) Q_DECL_OVERRIDE;
    void onConsoleMessageReceived(MessageSource source, MessageLevel level, const QString& message, int lineNumber, const QString& sourceID);
    QString userAgentForUrl(const QUrl& url) const Q_DECL_OVERRIDE;
    bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type) Q_DECL_OVERRIDE;
    QWebPage* createWindow(WebWindowType type) Q_DECL_OVERRIDE;

private:
    QPointer<WebPage> m_webPage;
    QPointer<CookieJar> m_cookieJar;
    QString m_userAgent;
    QStringList m_uploadFiles;
};
