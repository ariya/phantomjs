/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qfileselector.h"
#include "qfileselector_p.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QDebug>

#ifdef Q_OS_UNIX
#include <sys/utsname.h>
#endif

QT_BEGIN_NAMESPACE

//Environment variable to allow tooling full control of file selectors
static const char env_override[] = "QT_NO_BUILTIN_SELECTORS";

static const ushort selectorIndicator = '+';

Q_GLOBAL_STATIC(QFileSelectorSharedData, sharedData);
static QBasicMutex sharedDataMutex;

QFileSelectorPrivate::QFileSelectorPrivate()
    : QObjectPrivate()
{
}

/*!
    \class QFileSelector
    \inmodule QtCore
    \brief QFileSelector provides a convenient way of selecting file variants.
    \since 5.2

    QFileSelector is a convenience for selecting file variants based on platform or device
    characteristics. This allows you to develop and deploy one codebase containing all the
    different variants more easily in some circumstances, such as when the correct variant cannot
    be determined during the deploy step.

    \section1 Using QFileSelector

    If you always use the same file you do not need to use QFileSelector.

    Consider the following example usage, where you want to use different settings files on
    different locales. You might select code between locales like this:

    \code
    QString defaultsBasePath = "data/";
    QString defaultsPath = defaultsBasePath + "defaults.conf";
    QString localizedPath = defaultsBasePath
            + QString("%1/defaults.conf").arg(QLocale::system().name());
    if (QFile::exists(localizedPath))
        defaultsPath = localizedPath;
    QFile defaults(defaultsPath);
    \endcode

    Similarly, if you want to pick a different data file based on target platform,
    your code might look something like this:
    \code
    QString defaultsPath = "data/defaults.conf";
#if defined(Q_OS_ANDROID)
    defaultsPath = "data/android/defaults.conf";
#elif defined(Q_OS_BLACKBERRY)
    defaultsPath = "data/blackberry/defaults.conf";
#elif defined(Q_OS_IOS)
    defaultsPath = "data/ios/defaults.conf";
#endif
    QFile defaults(defaultsPath);
    \endcode

    QFileSelector provides a convenient alternative to writing such boilerplate code, and in the
    latter case it allows you to start using an platform-specific configuration without a recompile.
    QFileSelector also allows for chaining of multiple selectors in a convenient way, for example
    selecting a different file only on certain combinations of platform and locale. For example, to
    select based on platform and/or locale, the code is as follows:

    \code
    QFileSelector selector;
    QFile defaultsFile(selector.select("data/defaults.conf"));
    \endcode

    The files to be selected are placed in directories named with a \c'+' and a selector name. In the above
    example you could have the platform configurations selected by placing them in the following locations:
    \code
    data/defaults.conf
    data/+android/defaults.conf
    data/+blackberry/defaults.conf
    data/+ios/+en_GB/defaults.conf
    \endcode

    To find selected files, QFileSelector looks in the same directory as the base file. If there are
    any directories of the form +<selector> with an active selector, QFileSelector will prefer a file
    with the same file name from that directory over the base file. These directories can be nested to
    check against multiple selectors, for example:
    \code
    images/background.png
    images/+android/+en_GB/background.png
    images/+blackberry/+en_GB/background.png
    \endcode
    With those files available, you would select a different file on android and blackberry platforms,
    but only if the locale was en_GB.

    QFileSelector will not attempt to select if the base file does not exist. For error handling in
    the case no valid selectors are present, it is recommended to have a default or error-handling
    file in the base file location even if you expect selectors to be present for all deployments.

    In a future version, some may be marked as deploy-time static and be moved during the
    deployment step as an optimization. As selectors come with a performance cost, it is
    recommended to avoid their use in circumstances involving performance-critical code.

    \section1 Adding selectors

    Selectors normally available are
    \list
    \li platform, any of the following strings which match the platform the application is running
        on: android, blackberry, ios, osx, darwin, mac, linux, wince, unix, windows.
    \li locale, same as QLocale::system().name().
    \endlist

    Further selectors will be added from the \c QT_FILE_SELECTORS environment variable, which
    when set should be a set of comma separated selectors. Note that this variable will only be
    read once; selectors may not update if the variable changes while the application is running.
    The initial set of selectors are evaluated only once, on first use.

    You can also add extra selectors at runtime for custom behavior. These will be used in any
    future calls to select(). If the extra selectors list has been changed, calls to select() will
    use the new list and may return differently.

    \section1 Conflict resolution when multiple selectors apply

    When multiple selectors could be applied to the same file, the first matching selector is chosen.
    The order selectors are checked in are:

    \list 1
    \li Selectors set via setExtraSelectors(), in the order they are in the list
    \li Selectors in the \c QT_FILE_SELECTORS environment variable, from left to right
    \li Locale
    \li Platform
    \endlist

    Here is an example involving multiple selectors matching at the same time. It uses platform
    selectors, plus an extra selector named "admin" is set by the application based on user
    credentials. The example is sorted so that the lowest matching file would be chosen if all
    selectors were present:

    \code
    images/background.png
    images/+linux/background.png
    images/+windows/background.png
    images/+admin/background.png
    images/+admin/+linux/background.png
    \endcode

    Because extra selectors are checked before platform the \c{+admin/background.png} will be chosen
    on Windows when the admin selector is set, and \c{+windows/background.png} will be chosen on
    Windows when the admin selector is not set.  On Linux, the \c{+admin/+linux/background.png} will be
    chosen when admin is set, and the \c{+linux/background.png} when it is not.

*/

/*!
    Create a QFileSelector instance. This instance will have the same static selectors as other
    QFileSelector instances, but its own set of extra selectors.

    If supplied, it will have the given QObject \a parent.
*/
QFileSelector::QFileSelector(QObject *parent)
    : QObject(*(new QFileSelectorPrivate()), parent)
{
}

/*!
  Destroys this selector instance.
*/
QFileSelector::~QFileSelector()
{
}

/*!
   This function returns the selected version of the path, based on the conditions at runtime.
   If no selectable files are present, returns the original \a filePath.

   If the original file does not exist, the original \a filePath is returned. This means that you
   must have a base file to fall back on, you cannot have only files in selectable sub-directories.

   See the class overview for the selection algorithm.
*/
QString QFileSelector::select(const QString &filePath) const
{
    Q_D(const QFileSelector);
    return d->select(filePath);
}

static QString qrcScheme()
{
    return QStringLiteral("qrc");
}

/*!
   This is a convenience version of select operating on QUrls. If the scheme is not file or qrc,
   \a filePath is returned immediately. Otherwise selection is applied to the path of \a filePath
   and a QUrl is returned with the selected path and other QUrl parts the same as \a filePath.

   See the class overview for the selection algorithm.
*/
QUrl QFileSelector::select(const QUrl &filePath) const
{
    Q_D(const QFileSelector);
    if (filePath.scheme() != qrcScheme() && !filePath.isLocalFile())
        return filePath;
    QUrl ret(filePath);
    if (filePath.scheme() == qrcScheme()) {
        QString equivalentPath = QLatin1Char(':') + filePath.path();
        QString selectedPath = d->select(equivalentPath);
        ret.setPath(selectedPath.remove(0, 1));
    } else {
        ret = QUrl::fromLocalFile(d->select(ret.toLocalFile()));
    }
    return ret;
}

static QString selectionHelper(const QString &path, const QString &fileName, const QStringList &selectors)
{
    /* selectionHelper does a depth-first search of possible selected files. Because there is strict
       selector ordering in the API, we can stop checking as soon as we find the file in a directory
       which does not contain any other valid selector directories.
    */
    Q_ASSERT(path.isEmpty() || path.endsWith(QLatin1Char('/')));

    foreach (const QString &s, selectors) {
        QString prospectiveBase = path + QLatin1Char(selectorIndicator) + s + QLatin1Char('/');
        QStringList remainingSelectors = selectors;
        remainingSelectors.removeAll(s);
        if (!QDir(prospectiveBase).exists())
            continue;
        QString prospectiveFile = selectionHelper(prospectiveBase, fileName, remainingSelectors);
        if (!prospectiveFile.isEmpty())
            return prospectiveFile;
    }

    // If we reach here there were no successful files found at a lower level in this branch, so we
    // should check this level as a potential result.
    if (!QFile::exists(path + fileName))
        return QString();
    return path + fileName;
}

QString QFileSelectorPrivate::select(const QString &filePath) const
{
    Q_Q(const QFileSelector);
    QFileInfo fi(filePath);
    // If file doesn't exist, don't select
    if (!fi.exists())
        return filePath;

    QString ret = selectionHelper(fi.path().isEmpty() ? QString() : fi.path() + QLatin1Char('/'),
            fi.fileName(), q->allSelectors());

    if (!ret.isEmpty())
        return ret;
    return filePath;
}

/*!
    Returns the list of extra selectors which have been added programmatically to this instance.
*/
QStringList QFileSelector::extraSelectors() const
{
    Q_D(const QFileSelector);
    return d->extras;
}

/*!
    Sets the \a list of extra selectors which have been added programmatically to this instance.

    These selectors have priority over any which have been automatically picked up.
*/
void QFileSelector::setExtraSelectors(const QStringList &list)
{
    Q_D(QFileSelector);
    d->extras = list;
}

/*!
    Returns the complete, ordered list of selectors used by this instance
*/
QStringList QFileSelector::allSelectors() const
{
    Q_D(const QFileSelector);
    QMutexLocker locker(&sharedDataMutex);
    QFileSelectorPrivate::updateSelectors();
    return d->extras + sharedData->staticSelectors;
}

void QFileSelectorPrivate::updateSelectors()
{
    if (!sharedData->staticSelectors.isEmpty())
        return; //Already loaded

    QLatin1Char pathSep(',');
    QStringList envSelectors = QString::fromLatin1(qgetenv("QT_FILE_SELECTORS"))
                                .split(pathSep, QString::SkipEmptyParts);
    if (envSelectors.count())
        sharedData->staticSelectors << envSelectors;

    if (!qgetenv(env_override).isEmpty())
        return;

    sharedData->staticSelectors << sharedData->preloadedStatics; //Potential for static selectors from other modules

    // TODO: Update on locale changed?
    sharedData->staticSelectors << QLocale::system().name();

    sharedData->staticSelectors << platformSelectors();
}

QStringList QFileSelectorPrivate::platformSelectors()
{
    QStringList ret;
#if defined(Q_OS_WIN)
    ret << QStringLiteral("windows");
#  if defined(Q_OS_WINCE)
    ret << QStringLiteral("wince");
#  endif
#elif defined(Q_OS_UNIX)
    ret << QStringLiteral("unix");
#  if defined(Q_OS_ANDROID)
    ret << QStringLiteral("android");
#  elif defined(Q_OS_BLACKBERRY)
    ret << QStringLiteral("blackberry");
#  elif defined(Q_OS_QNX)
    ret << QStringLiteral("qnx");
#  elif defined(Q_OS_LINUX)
    ret << QStringLiteral("linux");
#  elif defined(Q_OS_DARWIN)
    ret << QStringLiteral("darwin");
    ret << QStringLiteral("mac"); // compatibility synonym
#    if defined(Q_OS_IOS)
    ret << QStringLiteral("ios");
#    elif defined(Q_OS_OSX)
    ret << QStringLiteral("osx");
#    endif
#  else
    struct utsname u;
    if (uname(&u) != -1)
        ret << QString::fromLatin1(u.sysname).toLower();
#  endif
#endif
    return ret;
}

void QFileSelectorPrivate::addStatics(const QStringList &statics)
{
    QMutexLocker locker(&sharedDataMutex);
    sharedData->preloadedStatics << statics;
}

QT_END_NAMESPACE

#include "moc_qfileselector.cpp"
