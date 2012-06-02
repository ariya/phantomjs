/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>

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

#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QMap>
#include <QVariantMap>
#include <QWebPage>
#include <QWebFrame>

#include "replcompletable.h"

class Config;
class CustomPage;
class NetworkAccessManager;
class QWebInspector;
class Phantom;

class WebPage: public REPLCompletable, public QWebFrame::PrintCallback
{
    Q_OBJECT
    Q_PROPERTY(QString content READ content WRITE setContent)
    Q_PROPERTY(QString plainText READ plainText)
    Q_PROPERTY(QString libraryPath READ libraryPath WRITE setLibraryPath)
    Q_PROPERTY(QString offlineStoragePath READ offlineStoragePath)
    Q_PROPERTY(int offlineStorageQuota READ offlineStorageQuota)
    Q_PROPERTY(QVariantMap viewportSize READ viewportSize WRITE setViewportSize)
    Q_PROPERTY(QVariantMap paperSize READ paperSize WRITE setPaperSize)
    Q_PROPERTY(QVariantMap clipRect READ clipRect WRITE setClipRect)
    Q_PROPERTY(QVariantMap scrollPosition READ scrollPosition WRITE setScrollPosition)
    Q_PROPERTY(QVariantMap customHeaders READ customHeaders WRITE setCustomHeaders)

public:
    WebPage(QObject *parent, const Config *config, const QUrl &baseUrl = QUrl());

    QWebFrame *mainFrame();

    QString content() const;
    void setContent(const QString &content);

    QString plainText() const;

    QString libraryPath() const;
    void setLibraryPath(const QString &dirPath);

    QString offlineStoragePath() const;

    int offlineStorageQuota() const;

    void setViewportSize(const QVariantMap &size);
    QVariantMap viewportSize() const;

    void setClipRect(const QVariantMap &size);
    QVariantMap clipRect() const;

    void setScrollPosition(const QVariantMap &size);
    QVariantMap scrollPosition() const;

    void setPaperSize(const QVariantMap &size);
    QVariantMap paperSize() const;

    void setCustomHeaders(const QVariantMap &headers);
    QVariantMap customHeaders() const;

    void showInspector(const int remotePort = -1);

    QString footer(int page, int numPages);
    qreal footerHeight() const;
    QString header(int page, int numPages);
    qreal headerHeight() const;

public slots:
    void openUrl(const QString &address, const QVariant &op, const QVariantMap &settings);
    void release();

    QVariant evaluateJavaScript(const QString &code);
    bool render(const QString &fileName);
    bool injectJs(const QString &jsFilePath);
    void _appendScriptElement(const QString &scriptUrl);
    void uploadFile(const QString &selector, const QString &fileName);
    void sendEvent(const QString &type, const QVariant &arg1 = QVariant(), const QVariant &arg2 = QVariant());

signals:
    void initialized();
    void loadStarted();
    void loadFinished(const QString &status);
    void javaScriptAlertSent(const QString &msg);
    void javaScriptConsoleMessageSent(const QString &message);
    void javaScriptErrorSent();
    void resourceRequested(const QVariant &req);
    void resourceReceived(const QVariant &resource);

private slots:
    void finish(bool ok);

private:
    QImage renderImage();
    bool renderPdf(const QString &fileName);
    void applySettings(const QVariantMap &defaultSettings);
    QString userAgent() const;

    void emitAlert(const QString &msg);
    void emitConsoleMessage(const QString &msg);
    void emitError();

    virtual void initCompletions();

private:
    CustomPage *m_webPage;
    NetworkAccessManager *m_networkAccessManager;
    QWebFrame *m_mainFrame;
    QRect m_clipRect;
    QPoint m_scrollPosition;
    QVariantMap m_paperSize; // For PDF output via render()
    QString m_libraryPath;
    QWebInspector* m_inspector;

    friend class Phantom;
    friend class CustomPage;

    QPoint m_mousePos;
};

#endif // WEBPAGE_H
