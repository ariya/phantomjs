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
    Q_PROPERTY(QString windowName READ windowName)
    Q_PROPERTY(QObjectList pages READ pages)
    Q_PROPERTY(QStringList pagesWindowName READ pagesWindowName)
    Q_PROPERTY(bool ownsPages READ ownsPages WRITE setOwnsPages)
    Q_PROPERTY(QStringList framesName READ framesName)
    Q_PROPERTY(QString frameName READ frameName)
    Q_PROPERTY(int framesCount READ framesCount)

public:
    WebPage(QObject *parent, const QUrl &baseUrl = QUrl());
    virtual ~WebPage();

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

    /**
     * Value of <code>"window.name"</code> within the main page frame.
     *
     * It's just a convenience method for
     * <code>"page.evaluate('return window.name;')"</code>
     *
     * @brief windowName
     * @return Returns the value of <code>'window.name'</code>
     *         within the current frame
     */
    QString windowName() const;

    /**
     * Returns a list of (Child) Pages that this page has currently open.
     * A page opens chilp pages when using <code>"window.open()"</code>.
     * If a child page spontaneously closes
     * (i.e. a call to <code>"window.close()"<code>) or it's manually closed
     * (i.e. "page.pages[i].release()"), the page is automatically removed by
     * this array.
     *
     * NOTE: The ownership of this array is held by the Page: it's not adviced
     * to have a "long running reference" to this array, as it might change.
     * NOTE: If "ownsPages()" is "false", the page will create pages but not
     * hold any ownership to it. Resource management is than left to the user.
     *
     * @brief pages
     * @return List (JS Array) containing the Pages that this page
     *         has currently open.
     */
    QObjectList pages() const;
    /**
     * Returns a list of (Child) Pages <code>"window.name"</code>.
     *
     * NOTE: When a page is opened with <code>"window.open"</code>, a window
     * <code>"name"</code> might be provided as second parameter.
     * This provides a useful list of those.
     * NOTE: If "ownsPages()" is "false", the page will create pages but not
     * hold any ownership of it. Resource management is than left to the user.
     *
     * @brief pagesWindowName
     * @return List (JS Array) containing the <code>'window.name'</code>(s) of
     *         Pages that this page has currently open.
     */
    QStringList pagesWindowName() const;

    /**
     * Returns "true" if it owns the pages it creates (and keeps them in "pages[]").
     * Default value is "true". Can be changed using {@link setOwnsPages()}.
     *
     * @brief ownsPages()
     * @return "true" if it owns the pages it creates in "pages[]", "false" otherwise.
     */
    bool ownsPages() const;
    /**
     * Set if, from now on, it should own the pages it creates in "pages[]".
     * Default value is "true".
     *
     * NOTE: When switching from "false" to "true", only the pages created
     * from that point on will be owned. It's NOT retroactive.
     *
     * @brief setOwnsPages
     * @param owns "true" to make it own the pages it creates in "pages[]", "false" otherwise.
     */
    void setOwnsPages(const bool owns);

    /**
     * Returns the number of Child Frames inside the Current Frame.
     * NOTE: The Current Frame changes when focus moves (via API or JS) to a specific child frame.
     *
     * @brief framesCount
     * @return Number of Frames inside the Current Frame
     */
    int framesCount() const;
    /**
     * Returns a list of (Child) Frames name.
     * NOTE: The Current Frame changes when focus moves (via API or JS) to a specific child frame.
     *
     * @brief framesName
     * @return List (JS Array) containing the names of the Child Frames inside the Current Frame (if any)
     */
    QStringList framesName() const;
    /**
     * Returns the name of the (Current) Frame
     *
     * @brief frameName
     * @return Name of the Current Frame
     */
    QString frameName() const;

public slots:
    void openUrl(const QString &address, const QVariant &op, const QVariantMap &settings);
    void release();
    void close();

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
     * Returns a Child Page that matches the given <code>"window.name"</code>.
     * This utility method is faster than accessing the
     * <code>"windowName"</code> property of every <code>"page.pages"</code>
     * and try to match.
     *
     * @brief getPage
     * @param windowName
     * @return Returns the page that matches <code>'window.name'</code>,
     *         or NULL if none is found
     */
    QObject *getPage(const QString &windowName) const;

    /**
     * Returns the number of Child Frames inside the Current Frame.
     * NOTE: The Current Frame changes when focus moves (via API or JS) to a specific child frame.
     *
     * @deprecated
     * @brief childFramesCount
     * @return Number of Frames inside the Current Frame
     */
    int childFramesCount() const;
    /**
     * Returns a list of Child Frames name.
     * NOTE: The Current Frame changes when focus moves (via API or JS) to a specific child frame.
     *
     * @deprecated
     * @brief childFramesName
     * @return List (JS Array) containing the names of the Child Frames inside the Current Frame (if any)
     */
    QStringList childFramesName() const;
    /**
     * Switches focus from the Current Frame to a Child Frame, identified by it's name.
     *
     * @brief switchToFrame
     * @param frameName Name of the Child frame
     * @return "true" if the frame was found, "false" otherwise
     */
    bool switchToFrame(const QString &frameName);
    /**
     * Switches focus from the Current Frame to a Child Frame, identified by it's name.
     *
     * @deprecated
     * @brief switchToChildFrame
     * @param frameName Name of the Child frame
     * @return "true" if the frame was found, "false" otherwise
     */
    bool switchToChildFrame(const QString &frameName);
    /**
     * Switches focus from the Current Frame to a Child Frame, identified by it positional order.
     *
     * @brief switchToFrame
     * @param framePosition Position of the Frame inside the Child Frames array (i.e. "window.frames[i]")
     * @return "true" if the frame was found, "false" otherwise
     */
    bool switchToFrame(const int framePosition);
    /**
     * Switches focus from the Current Frame to a Child Frame, identified by it positional order.
     *
     * @deprecated
     * @brief switchToChildFrame
     * @param framePosition Position of the Frame inside the Child Frames array (i.e. "window.frames[i]")
     * @return "true" if the frame was found, "false" otherwise
     */
    bool switchToChildFrame(const int framePosition);
    /**
     * Switches focus to the Main Frame within this Page.
     *
     * @brief switchToMainFrame
     */
    void switchToMainFrame();
    /**
     * Switches focus to the Parent Frame of the Current Frame (if it exists).
     *
     * @brief switchToParentFrame
     * @return "true" if the Current Frame is not a Main Frame, "false" otherwise (i.e. there is no parent frame to switch to)
     */
    bool switchToParentFrame();
    /**
     * Returns the name of the Current Frame (if it has one)
     *
     * @deprecated
     * @brief currentFrameName
     * @return Name of the Current Frame
     */
    QString currentFrameName() const;

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
    void rawPageCreated(QObject *page);
    void closing(QObject *page);

private slots:
    void finish(bool ok);
    void handleJavaScriptWindowObjectCleared();

private:
    QImage renderImage();
    bool renderPdf(const QString &fileName);
    void applySettings(const QVariantMap &defaultSettings);
    QString userAgent() const;

    bool javaScriptConfirm(const QString &msg);
    bool javaScriptPrompt(const QString &msg, const QString &defaultValue, QString *result);

    virtual void initCompletions();

private:
    CustomPage *m_customWebPage;
    NetworkAccessManager *m_networkAccessManager;
    QWebFrame *m_mainFrame;
    QRect m_clipRect;
    QPoint m_scrollPosition;
    QVariantMap m_paperSize; // For PDF output via render()
    QString m_libraryPath;
    QWebInspector* m_inspector;
    WebpageCallbacks *m_callbacks;
    bool m_navigationLocked;
    QPoint m_mousePos;
    bool m_ownsPages;

    friend class Phantom;
    friend class CustomPage;
};

#endif // WEBPAGE_H
