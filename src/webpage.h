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
class WebpageCallbacks;
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
    Q_PROPERTY(bool navigationLocked READ navigationLocked WRITE setNavigationLocked)
    Q_PROPERTY(QVariantMap customHeaders READ customHeaders WRITE setCustomHeaders)
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor)
    Q_PROPERTY(QVariantList cookies READ cookies WRITE setCookies)

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

    void setNavigationLocked(bool lock);
    bool navigationLocked();

    void setCustomHeaders(const QVariantMap &headers);
    QVariantMap customHeaders() const;

    void showInspector(const int remotePort = -1);

    QString footer(int page, int numPages);
    qreal footerHeight() const;
    QString header(int page, int numPages);
    qreal headerHeight() const;

    void setZoomFactor(qreal zoom);
    qreal zoomFactor() const;

public slots:
    void openUrl(const QString &address, const QVariant &op, const QVariantMap &settings);
    void release();

    QVariant evaluateJavaScript(const QString &code);
    bool render(const QString &fileName);
    /**
     * Render the page as base-64 encoded string.
     * Default image format is "png".
     *
     * To choose a different format, pass a string with it's name.
     * Available formats are the one supported by Qt QImageWriter class:
     * @link http://qt-project.org/doc/qt-4.8/qimagewriter.html#supportedImageFormats.
     *
     * @brief renderBase64
     * @param format String containing one of the supported types
     * @return Rendering base-64 encoded of the page if the given format is supported, otherwise an empty string
     */
    QString renderBase64(const QByteArray &format = "png");
    bool injectJs(const QString &jsFilePath);
    void _appendScriptElement(const QString &scriptUrl);
    QObject *_getGenericCallback();
    QObject *_getJsConfirmCallback();
    QObject *_getJsPromptCallback();
    void uploadFile(const QString &selector, const QString &fileName);
    void sendEvent(const QString &type, const QVariant &arg1 = QVariant(), const QVariant &arg2 = QVariant());

    /**
     * Returns the number of Child Frames inside the Current Frame.
     * NOTE: The Current Frame changes when focus moves (via API or JS) to a specific child frame.
     * @brief childFramesCount
     * @return Number of Frames inside the Current Frame
     */
    int childFramesCount();
    /**
     * Returns a list of Child Frames name.
     * NOTE: The Current Frame changes when focus moves (via API or JS) to a specific child frame.
     * @brief childFramesName
     * @return List (JS Array) containing the names of the Child Frames inside the Current Frame (if any)
     */
    QVariantList childFramesName();
    /**
     * Switches focus from the Current Frame to a Child Frame, identified by it's name.
     * @brief switchToChildFrame
     * @param frameName Name of the Child frame
     * @return "true" if the frame was found, "false" otherwise
     */
    bool switchToChildFrame(const QString &frameName);
    /**
     * Switches focus from the Current Frame to a Child Frame, identified by it positional order.
     * @brief switchToChildFrame
     * @param framePosition Position of the Frame inside the Child Frames array (i.e. "window.frames[i]")
     * @return "true" if the frame was found, "false" otherwise
     */
    bool switchToChildFrame(const int framePosition);
    /**
     * Switches focus to the Main Frame within this Page.
     * @brief switchToMainFrame
     */
    void switchToMainFrame();
    /**
     * Switches focus to the Parent Frame of the Current Frame (if it exists).
     * @brief switchToParentFrame
     * @return "true" if the Current Frame is not a Main Frame, "false" otherwise (i.e. there is no parent frame to switch to)
     */
    bool switchToParentFrame();
    /**
     * Returns the name of the Current Frame (if it has one)
     * @brief currentFrameName
     * @return Name of the Current Frame
     */
    QString currentFrameName();

    void setCookies(const QVariantList &cookies);
    QVariantList cookies() const;

signals:
    void initialized();
    void loadStarted();
    void loadFinished(const QString &status);
    void javaScriptAlertSent(const QString &msg);
    void javaScriptConsoleMessageSent(const QString &message);
    void javaScriptErrorSent(const QString &msg, const QString &stack);
    void resourceRequested(const QVariant &req);
    void resourceReceived(const QVariant &resource);
    void urlChanged(const QUrl &url);
    void navigationRequested(const QUrl &url, const QString &navigationType, bool navigationLocked, bool isMainFrame);

private slots:
    void finish(bool ok);
    void handleJavaScriptWindowObjectCleared();

private:
    QImage renderImage();
    bool renderPdf(const QString &fileName);
    void applySettings(const QVariantMap &defaultSettings);
    QString userAgent() const;

    void emitAlert(const QString &msg);
    void emitConsoleMessage(const QString &msg);
    void emitError(const QString &msg, const QString &stack);

    bool javaScriptConfirm(const QString &msg);
    bool javaScriptPrompt(const QString &msg, const QString &defaultValue, QString *result);

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
    WebpageCallbacks *m_callbacks;
    bool m_navigationLocked;

    friend class Phantom;
    friend class CustomPage;

    QPoint m_mousePos;
};

#endif // WEBPAGE_H
