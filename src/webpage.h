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
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKitWidgets/QWebFrame>
#include <QPdfWriter>

#include "cookiejar.h"

class Config;
class CustomPage;
class WebpageCallbacks;
class NetworkAccessManager;
class QWebInspector;
class Phantom;

class WebPage : public QObject, public QWebFrame::PrintCallback
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QString frameTitle READ frameTitle)
    Q_PROPERTY(QString content READ content WRITE setContent)
    Q_PROPERTY(QString frameContent READ frameContent WRITE setFrameContent)
    Q_PROPERTY(QString url READ url)
    Q_PROPERTY(QString frameUrl READ frameUrl)
    Q_PROPERTY(bool loading READ loading)
    Q_PROPERTY(int loadingProgress READ loadingProgress)
    Q_PROPERTY(bool canGoBack READ canGoBack)
    Q_PROPERTY(bool canGoForward READ canGoForward)
    Q_PROPERTY(QString plainText READ plainText)
    Q_PROPERTY(QString framePlainText READ framePlainText)
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
    Q_PROPERTY(QString focusedFrameName READ focusedFrameName)
    Q_PROPERTY(QObject* cookieJar READ cookieJar WRITE setCookieJarFromQObject)

public:
    WebPage(QObject* parent, const QUrl& baseUrl = QUrl());
    virtual ~WebPage();

    QWebFrame* mainFrame();

    QString content() const;
    QString frameContent() const;
    void setContent(const QString& content);
    void setFrameContent(const QString& content);

    QString title() const;
    QString frameTitle() const;

    QString url() const;
    QString frameUrl() const;

    bool loading() const;
    int loadingProgress() const;

    QString plainText() const;
    QString framePlainText() const;

    QString libraryPath() const;
    void setLibraryPath(const QString& dirPath);

    QString offlineStoragePath() const;

    int offlineStorageQuota() const;

    void setViewportSize(const QVariantMap& size);
    QVariantMap viewportSize() const;

    void setClipRect(const QVariantMap& size);
    QVariantMap clipRect() const;

    void setScrollPosition(const QVariantMap& size);
    QVariantMap scrollPosition() const;

    void setPaperSize(const QVariantMap& size);
    QVariantMap paperSize() const;

    void setNavigationLocked(bool lock);
    bool navigationLocked();

    void setCustomHeaders(const QVariantMap& headers);
    QVariantMap customHeaders() const;

    int showInspector(const int remotePort = -1);

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
    /**
     * Returns the currently focused Frame's name.
     *
     * @brief focusedFrameName
     * @return Frame
     */
    QString focusedFrameName() const;

public slots:
    void openUrl(const QString& address, const QVariant& op, const QVariantMap& settings);
    void release();
    void close();

    QVariant evaluateJavaScript(const QString& code);
    bool render(const QString& fileName, const QVariantMap& map = QVariantMap());
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
    QString renderBase64(const QByteArray& format = "png");
    bool injectJs(const QString& jsFilePath);
    void _appendScriptElement(const QString& scriptUrl);
    QObject* _getGenericCallback();
    QObject* _getFilePickerCallback();
    QObject* _getJsConfirmCallback();
    QObject* _getJsPromptCallback();
    QObject* _getJsInterruptCallback();
    void _uploadFile(const QString& selector, const QStringList& fileNames);
    void sendEvent(const QString& type, const QVariant& arg1 = QVariant(), const QVariant& arg2 = QVariant(), const QString& mouseButton = QString(), const QVariant& modifierArg = QVariant());

    void setContent(const QString& content, const QString& baseUrl);
    void setFrameContent(const QString& content, const QString& baseUrl);
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
    QObject* getPage(const QString& windowName) const;

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
    bool switchToFrame(const QString& frameName);
    /**
     * Switches focus from the Current Frame to a Child Frame, identified by it's name.
     *
     * @deprecated
     * @brief switchToChildFrame
     * @param frameName Name of the Child frame
     * @return "true" if the frame was found, "false" otherwise
     */
    bool switchToChildFrame(const QString& frameName);
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
     * Switches to the currently focused frame, as per QWebPage.  This is the frame whose
     * window element was last focus()ed, and is currently the target of key events.
     *
     * @brief switchToFocusedFrame
     */
    void switchToFocusedFrame();
    /**
     * Returns the name of the Current Frame (if it has one)
     *
     * @deprecated
     * @brief currentFrameName
     * @return Name of the Current Frame
     */
    QString currentFrameName() const;

    /**
     * Allows to set cookie jar for this page.
     */
    void setCookieJar(CookieJar* cookieJar);

    /**
     * Allows to set cookie jar in through QtWebKit Bridge
     */
    void setCookieJarFromQObject(QObject* cookieJar);

    /**
     * Returns the CookieJar object
     */
    CookieJar* cookieJar();

    /**
     * Allows to set cookies by this Page, at the current URL.
     * This means that loading new URLs, causes the cookies to change dynamically
     * as in a normal desktop browser.
     *
     * Cookies are expected in the format:
     * <pre>
     * {
     *   "name"     : "cookie name (string)",
     *   "value"    : "cookie value (string)",
     *   "domain"   : "cookie domain (string)",
     *   "path"     : "cookie path (string, optional)",
     *   "httponly" : "http only cookie (boolean, optional)",
     *   "secure"   : "secure cookie (boolean, optional)",
     *   "expires"  : "expiration date (string, GMT format, optional)"
     * }
     * </pre>
     * @brief setCookies
     * @param cookies Expects a QList of QVariantMaps
     * @return Boolean "true" if at least 1 cookie was set
     */
    bool setCookies(const QVariantList& cookies);
    /**
     * Cookies visible by this Page, at the current URL.
     *
     * @see WebPage::setCookies for details on the format
     * @brief cookies
     * @return QList of QVariantMap cookies visible to this Page, at the current URL.
     */
    QVariantList cookies() const;
    /**
     * Add a Cookie in QVariantMap format
     * @see WebPage::setCookies for details on the format
     * @brief addCookie
     * @param cookie Cookie in QVariantMap format
     * @return Boolean "true" if cookie was added
     */
    bool addCookie(const QVariantMap& cookie);
    /**
     * Delete cookie by name from the ones visible by this Page, at the current URL
     * @brief deleteCookie
     * @param cookieName Name of the Cookie to delete
     * @return Boolean "true" if cookie was deleted
     */
    bool deleteCookie(const QString& cookieName);
    /**
     * Delete All Cookies visible by this Page, at the current URL
     * @brief clearCookies
     * @return Boolean "true" if cookies were deleted
     */
    bool clearCookies();

    /**
     * Checks if this Page can go back in the Navigation History
     * @brief canGoBack
     * @return "true" if it can, "false" otherwise
     */
    bool canGoBack();
    /**
     * Goes back in the Navigation History
     * @brief goBack
     * @return "true" if it does go back in the Navigation History, "false" otherwise
     */
    bool goBack();
    /**
     * Checks if this Page can go forward in the Navigation History (i.e. next URL)
     * @brief canGoForward
     * @return "true" if it can, "false" otherwise
     */
    bool canGoForward();
    /**
     * Goes forward in the Navigation History
     * @brief goForward
     * @return "true" if it does go forward in the Navigation History, "false" otherwise
     */
    bool goForward();
    /**
     * Go to the page identified by its relative location to the current page.
     * For example '-1' for the previous page or 1 for the next page.
     *
     * Modelled after JavaScript "window.go(num)" method:
     * {@see https://developer.mozilla.org/en-US/docs/DOM/window.history#Syntax}.
     * @brief go
     * @param historyRelativeIndex
     * @return "true" if it does go forward/backgward in the Navigation History, "false" otherwise
     */
    bool go(int historyRelativeIndex);
    /**
     * Reload current page
     * @brief reload
     */
    void reload();
    /**
     * Stop loading page (if the page is loading)
     *
     * NOTE: This method does nothing when page is not actually loading.
     * It's effect can be applied in that very short window of time between
     * "onLoadStarted" and "onLoadFinished".
     *
     * @brief stop
     */
    void stop();

    void stopJavaScript();

    void clearMemoryCache();

    void setProxy(const QString& proxyUrl);

    qreal stringToPointSize(const QString&) const;
    qreal printMargin(const QVariantMap&, const QString&);
    qreal getHeight(const QVariantMap&, const QString&) const;

signals:
    void initialized();
    void loadStarted();
    void loadFinished(const QString& status);
    void javaScriptAlertSent(const QString& msg);
    void javaScriptConsoleMessageSent(const QString& message);
    void javaScriptErrorSent(const QString& msg, int lineNumber, const QString& sourceID, const QString& stack);
    void resourceRequested(const QVariant& requestData, QObject* request);
    void resourceReceived(const QVariant& resource);
    void resourceError(const QVariant& errorData);
    void resourceTimeout(const QVariant& errorData);
    void urlChanged(const QString& url);
    void navigationRequested(const QString& url, const QString& navigationType, bool navigationLocked, bool isMainFrame);
    void rawPageCreated(QObject* page);
    void closing(QObject* page);
    void repaintRequested(const int x, const int y, const int width, const int height);

private slots:
    void finish(bool ok);
    void setupFrame(QWebFrame* frame = NULL);
    void updateLoadingProgress(int progress);
    void handleRepaintRequested(const QRect& dirtyRect);
    void handleUrlChanged(const QUrl& url);
    void handleCurrentFrameDestroyed();

private:
    enum RenderMode { Content, Viewport };
    QImage renderImage(const RenderMode mode = Content);
    bool renderPdf(QPdfWriter& pdfWriter);
    void applySettings(const QVariantMap& defaultSettings);
    QString userAgent() const;

    /**
     * Switches focus from the Current Frame to the Child Frame, identified by `frame`.
     *
     * @brief changeCurrentFrame
     * @param frame The Child frame
     */
    void changeCurrentFrame(QWebFrame* const frame);

    QString filePicker(const QString& oldFile);
    bool javaScriptConfirm(const QString& msg);
    bool javaScriptPrompt(const QString& msg, const QString& defaultValue, QString* result);
    void javascriptInterrupt();

private:
    CustomPage* m_customWebPage;
    NetworkAccessManager* m_networkAccessManager;
    QWebFrame* m_mainFrame;
    QWebFrame* m_currentFrame;
    QRect m_clipRect;
    QPoint m_scrollPosition;
    QVariantMap m_paperSize; // For PDF output via render()
    QString m_libraryPath;
    QWebInspector* m_inspector;
    WebpageCallbacks* m_callbacks;
    bool m_navigationLocked;
    QPoint m_mousePos;
    bool m_ownsPages;
    int m_loadingProgress;
    bool m_shouldInterruptJs;
    CookieJar* m_cookieJar;
    qreal m_dpi;

    friend class Phantom;
    friend class CustomPage;
};

#endif // WEBPAGE_H
