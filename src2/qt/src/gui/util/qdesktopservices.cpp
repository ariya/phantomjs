/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdesktopservices.h"

#ifndef QT_NO_DESKTOPSERVICES

#include <qdebug.h>

#if defined(Q_OS_BLACKBERRY)
#include "qdesktopservices_blackberry.cpp"
#elif defined(Q_WS_QWS) || defined(Q_WS_QPA)
#include "qdesktopservices_qws.cpp"
#elif defined(Q_WS_X11)
#include "qdesktopservices_x11.cpp"
#elif defined(Q_WS_WIN)
#include "qdesktopservices_win.cpp"
#elif defined(Q_WS_MAC)
#include "qdesktopservices_mac.cpp"
#elif defined(Q_OS_SYMBIAN)
#include "qdesktopservices_s60.cpp"
#endif

#include <qhash.h>
#include <qobject.h>
#include <qcoreapplication.h>
#include <qurl.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

class QOpenUrlHandlerRegistry : public QObject
{
    Q_OBJECT
public:
    inline QOpenUrlHandlerRegistry() : mutex(QMutex::Recursive) {}

    QMutex mutex;

    struct Handler
    {
        QObject *receiver;
        QByteArray name;
    };
    typedef QHash<QString, Handler> HandlerHash;
    HandlerHash handlers;

public Q_SLOTS:
    void handlerDestroyed(QObject *handler);

};

Q_GLOBAL_STATIC(QOpenUrlHandlerRegistry, handlerRegistry)

void QOpenUrlHandlerRegistry::handlerDestroyed(QObject *handler)
{
    HandlerHash::Iterator it = handlers.begin();
    while (it != handlers.end()) {
        if (it->receiver == handler) {
            it = handlers.erase(it);
        } else {
            ++it;
        }
    }
}

/*!
    \class QDesktopServices
    \brief The QDesktopServices class provides methods for accessing common desktop services.
    \since 4.2
    \ingroup desktop

    Many desktop environments provide services that can be used by applications to
    perform common tasks, such as opening a web page, in a way that is both consistent
    and takes into account the user's application preferences.

    This class contains functions that provide simple interfaces to these services
    that indicate whether they succeeded or failed.

    The openUrl() function is used to open files located at arbitrary URLs in external
    applications. For URLs that correspond to resources on the local filing system
    (where the URL scheme is "file"), a suitable application will be used to open the
    file; otherwise, a web browser will be used to fetch and display the file.

    The user's desktop settings control whether certain executable file types are
    opened for browsing, or if they are executed instead. Some desktop environments
    are configured to prevent users from executing files obtained from non-local URLs,
    or to ask the user's permission before doing so.

    \section1 URL Handlers

    The behavior of the openUrl() function can be customized for individual URL
    schemes to allow applications to override the default handling behavior for
    certain types of URLs.

    The dispatch mechanism allows only one custom handler to be used for each URL
    scheme; this is set using the setUrlHandler() function. Each handler is
    implemented as a slot which accepts only a single QUrl argument.

    The existing handlers for each scheme can be removed with the
    unsetUrlHandler() function. This returns the handling behavior for the given
    scheme to the default behavior.

    This system makes it easy to implement a help system, for example. Help could be
    provided in labels and text browsers using \gui{help://myapplication/mytopic}
    URLs, and by registering a handler it becomes possible to display the help text
    inside the application:

    \snippet doc/src/snippets/code/src_gui_util_qdesktopservices.cpp 0

    If inside the handler you decide that you can't open the requested
    URL, you can just call QDesktopServices::openUrl() again with the
    same argument, and it will try to open the URL using the
    appropriate mechanism for the user's desktop environment.

    \sa QSystemTrayIcon, QProcess
*/

/*!
    Opens the given \a url in the appropriate Web browser for the user's desktop
    environment, and returns true if successful; otherwise returns false.

    If the URL is a reference to a local file (i.e., the URL scheme is "file") then
    it will be opened with a suitable application instead of a Web browser.

    The following example opens a file on the Windows file system residing on a path
    that contains spaces:

    \snippet doc/src/snippets/code/src_gui_util_qdesktopservices.cpp 2

    If a \c mailto URL is specified, the user's e-mail client will be used to open a
    composer window containing the options specified in the URL, similar to the way
    \c mailto links are handled by a Web browser.

    For example, the following URL contains a recipient (\c{user@foo.com}), a
    subject (\c{Test}), and a message body (\c{Just a test}):

    \snippet doc/src/snippets/code/src_gui_util_qdesktopservices.cpp 1

    \warning Although many e-mail clients can send attachments and are
    Unicode-aware, the user may have configured their client without these features.
    Also, certain e-mail clients (e.g., Lotus Notes) have problems with long URLs.

    \note On Symbian OS, \c SwEvent capability is required to open the given \a url
    if the Web browser is already running.

    \sa setUrlHandler()
*/
bool QDesktopServices::openUrl(const QUrl &url)
{
    QOpenUrlHandlerRegistry *registry = handlerRegistry();
    QMutexLocker locker(&registry->mutex);
    static bool insideOpenUrlHandler = false;

    if (!insideOpenUrlHandler) {
        QOpenUrlHandlerRegistry::HandlerHash::ConstIterator handler = registry->handlers.constFind(url.scheme());
        if (handler != registry->handlers.constEnd()) {
            insideOpenUrlHandler = true;
            bool result = QMetaObject::invokeMethod(handler->receiver, handler->name.constData(), Qt::DirectConnection, Q_ARG(QUrl, url));
            insideOpenUrlHandler = false;
            return result; // ### support bool slot return type
        }
    }

    bool result;
    if (url.scheme() == QLatin1String("file"))
        result = openDocument(url);
    else
        result = launchWebBrowser(url);

    return result;
}

/*!
    Sets the handler for the given \a scheme to be the handler \a method provided by
    the \a receiver object.

    This function provides a way to customize the behavior of openUrl(). If openUrl()
    is called with a URL with the specified \a scheme then the given \a method on the
    \a receiver object is called instead of QDesktopServices launching an external
    application.

    The provided method must be implemented as a slot that only accepts a single QUrl
    argument.

    If setUrlHandler() is used to set a new handler for a scheme which already
    has a handler, the existing handler is simply replaced with the new one.
    Since QDesktopServices does not take ownership of handlers, no objects are
    deleted when a handler is replaced.

    Note that the handler will always be called from within the same thread that
    calls QDesktopServices::openUrl().

    \sa openUrl(), unsetUrlHandler()
*/
void QDesktopServices::setUrlHandler(const QString &scheme, QObject *receiver, const char *method)
{
    QOpenUrlHandlerRegistry *registry = handlerRegistry();
    QMutexLocker locker(&registry->mutex);
    if (!receiver) {
        registry->handlers.remove(scheme);
        return;
    }
    QOpenUrlHandlerRegistry::Handler h;
    h.receiver = receiver;
    h.name = method;
    registry->handlers.insert(scheme, h);
    QObject::connect(receiver, SIGNAL(destroyed(QObject*)),
                     registry, SLOT(handlerDestroyed(QObject*)));
}

/*!
    Removes a previously set URL handler for the specified \a scheme.

    \sa setUrlHandler()
*/
void QDesktopServices::unsetUrlHandler(const QString &scheme)
{
    setUrlHandler(scheme, 0, 0);
}

/*!
    \enum QDesktopServices::StandardLocation
    \since 4.4

    This enum describes the different locations that can be queried by
    QDesktopServices::storageLocation and QDesktopServices::displayName.

    \value DesktopLocation Returns the user's desktop directory.
    \value DocumentsLocation Returns the user's document.
    \value FontsLocation Returns the user's fonts.
    \value ApplicationsLocation Returns the user's applications.
    \value MusicLocation Returns the users music.
    \value MoviesLocation Returns the user's movies.
    \value PicturesLocation Returns the user's pictures.
    \value TempLocation Returns the system's temporary directory.
    \value HomeLocation Returns the user's home directory.
    \value DataLocation Returns a directory location where persistent
           application data can be stored. QCoreApplication::applicationName
           and QCoreApplication::organizationName should work on all
           platforms.
    \value CacheLocation Returns a directory location where user-specific
           non-essential (cached) data should be written.

    \sa storageLocation() displayName()
*/

/*!
    \fn QString QDesktopServices::storageLocation(StandardLocation type)
    \since 4.4

    Returns the default system directory where files of \a type belong, or an empty string
    if the location cannot be determined.

    \note The storage location returned can be a directory that does not exist; i.e., it
    may need to be created by the system or the user.

    \note On Symbian OS, ApplicationsLocation always point /sys/bin folder on the same drive
    with executable. FontsLocation always points to folder on ROM drive. Symbian OS does not
    have desktop concept, DesktopLocation returns same path as DocumentsLocation.
    Rest of the standard locations point to folder on same drive with executable, except
    that if executable is in ROM the folder from C drive is returned.
*/

/*!
    \fn QString QDesktopServices::displayName(StandardLocation type)

    Returns a localized display name for the given location \a type or
    an empty QString if no relevant location can be found.
*/

QT_END_NAMESPACE

#include "qdesktopservices.moc"

#endif // QT_NO_DESKTOPSERVICES
