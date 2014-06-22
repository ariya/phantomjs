/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmimetype.h"

#include "qmimetype_p.h"
#include "qmimedatabase_p.h"
#include "qmimeprovider_p.h"

#include "qmimeglobpattern_p.h"

#include <QtCore/QDebug>
#include <QtCore/QLocale>

#include <memory>

QT_BEGIN_NAMESPACE

QMimeTypePrivate::QMimeTypePrivate()
    : loaded(false)
{}

QMimeTypePrivate::QMimeTypePrivate(const QMimeType &other)
        : name(other.d->name),
        localeComments(other.d->localeComments),
        genericIconName(other.d->genericIconName),
        iconName(other.d->iconName),
        globPatterns(other.d->globPatterns),
        loaded(other.d->loaded)
{}

void QMimeTypePrivate::clear()
{
    name.clear();
    localeComments.clear();
    genericIconName.clear();
    iconName.clear();
    globPatterns.clear();
    loaded = false;
}

void QMimeTypePrivate::addGlobPattern(const QString &pattern)
{
    globPatterns.append(pattern);
}

/*!
    \class QMimeType
    \inmodule QtCore
    \ingroup shared
    \brief The QMimeType class describes types of file or data, represented by a MIME type string.

    \since 5.0

    For instance a file named "readme.txt" has the MIME type "text/plain".
    The MIME type can be determined from the file name, or from the file
    contents, or from both. MIME type determination can also be done on
    buffers of data not coming from files.

    Determining the MIME type of a file can be useful to make sure your
    application supports it. It is also useful in file-manager-like applications
    or widgets, in order to display an appropriate icon() for the file, or even
    the descriptive comment() in detailed views.

    To check if a file has the expected MIME type, you should use inherits()
    rather than a simple string comparison based on the name(). This is because
    MIME types can inherit from each other: for instance a C source file is
    a specific type of plain text file, so text/x-csrc inherits text/plain.

    \sa QMimeDatabase
 */

/*!
    \fn QMimeType &QMimeType::operator=(QMimeType &&other)

    Move-assigns \a other to this QMimeType instance.

    \since 5.2
*/

/*!
    \fn QMimeType::QMimeType();
    Constructs this QMimeType object initialized with default property values that indicate an invalid MIME type.
 */
QMimeType::QMimeType() :
        d(new QMimeTypePrivate())
{
}

/*!
    \fn QMimeType::QMimeType(const QMimeType &other);
    Constructs this QMimeType object as a copy of \a other.
 */
QMimeType::QMimeType(const QMimeType &other) :
        d(other.d)
{
}

/*!
    \fn QMimeType &QMimeType::operator=(const QMimeType &other);
    Assigns the data of \a other to this QMimeType object, and returns a reference to this object.
 */
QMimeType &QMimeType::operator=(const QMimeType &other)
{
    if (d != other.d)
        d = other.d;
    return *this;
}

/*!
    \fn QMimeType::QMimeType(const QMimeTypePrivate &dd);
    Assigns the data of the QMimeTypePrivate \a dd to this QMimeType object, and returns a reference to this object.
    \internal
 */
QMimeType::QMimeType(const QMimeTypePrivate &dd) :
        d(new QMimeTypePrivate(dd))
{
}

/*!
    \fn void QMimeType::swap(QMimeType &other);
    Swaps QMimeType \a other with this QMimeType object.

    This operation is very fast and never fails.

    The swap() method helps with the implementation of assignment
    operators in an exception-safe way. For more information consult
    \l {http://en.wikibooks.org/wiki/More_C++_Idioms/Copy-and-swap}
    {More C++ Idioms - Copy-and-swap}.
 */

/*!
    \fn QMimeType::~QMimeType();
    Destroys the QMimeType object, and releases the d pointer.
 */
QMimeType::~QMimeType()
{
}

/*!
    \fn bool QMimeType::operator==(const QMimeType &other) const;
    Returns \c true if \a other equals this QMimeType object, otherwise returns \c false.
    The name is the unique identifier for a mimetype, so two mimetypes with
    the same name, are equal.
 */
bool QMimeType::operator==(const QMimeType &other) const
{
    return d == other.d || d->name == other.d->name;
}

/*!
    \fn bool QMimeType::operator!=(const QMimeType &other) const;
    Returns \c true if \a other does not equal this QMimeType object, otherwise returns \c false.
 */

/*!
    \fn bool QMimeType::isValid() const;
    Returns \c true if the QMimeType object contains valid data, otherwise returns \c false.
    A valid MIME type has a non-empty name().
    The invalid MIME type is the default-constructed QMimeType.
 */
bool QMimeType::isValid() const
{
    return !d->name.isEmpty();
}

/*!
    \fn bool QMimeType::isDefault() const;
    Returns \c true if this MIME type is the default MIME type which
    applies to all files: application/octet-stream.
 */
bool QMimeType::isDefault() const
{
    return d->name == QMimeDatabasePrivate::instance()->defaultMimeType();
}

/*!
    \fn QString QMimeType::name() const;
    Returns the name of the MIME type.
 */
QString QMimeType::name() const
{
    return d->name;
}

/*!
    Returns the description of the MIME type to be displayed on user interfaces.

    The system language (QLocale::system().name()) is used to select the appropriate translation.
 */
QString QMimeType::comment() const
{
    QMimeDatabasePrivate::instance()->provider()->loadMimeTypePrivate(*d);

    QStringList languageList;
    languageList << QLocale::system().name();
    languageList << QLocale::system().uiLanguages();
    Q_FOREACH (const QString &language, languageList) {
        const QString lang = language == QLatin1String("C") ? QLatin1String("en_US") : language;
        const QString comm = d->localeComments.value(lang);
        if (!comm.isEmpty())
            return comm;
        const int pos = lang.indexOf(QLatin1Char('_'));
        if (pos != -1) {
            // "pt_BR" not found? try just "pt"
            const QString shortLang = lang.left(pos);
            const QString commShort = d->localeComments.value(shortLang);
            if (!commShort.isEmpty())
                return commShort;
        }
    }

    // Use the mimetype name as fallback
    return d->name;
}

/*!
    \fn QString QMimeType::genericIconName() const;
    Returns the file name of a generic icon that represents the MIME type.

    This should be used if the icon returned by iconName() cannot be found on
    the system. It is used for categories of similar types (like spreadsheets
    or archives) that can use a common icon.
    The freedesktop.org Icon Naming Specification lists a set of such icon names.

    The icon name can be given to QIcon::fromTheme() in order to load the icon.
 */
QString QMimeType::genericIconName() const
{
    QMimeDatabasePrivate::instance()->provider()->loadGenericIcon(*d);
    if (d->genericIconName.isEmpty()) {
        // From the spec:
        // If the generic icon name is empty (not specified by the mimetype definition)
        // then the mimetype is used to generate the generic icon by using the top-level
        // media type (e.g.  "video" in "video/ogg") and appending "-x-generic"
        // (i.e. "video-x-generic" in the previous example).
        QString group = name();
        const int slashindex = group.indexOf(QLatin1Char('/'));
        if (slashindex != -1)
            group = group.left(slashindex);
        return group + QLatin1String("-x-generic");
    }
    return d->genericIconName;
}

/*!
    \fn QString QMimeType::iconName() const;
    Returns the file name of an icon image that represents the MIME type.

    The icon name can be given to QIcon::fromTheme() in order to load the icon.
 */
QString QMimeType::iconName() const
{
    QMimeDatabasePrivate::instance()->provider()->loadIcon(*d);
    if (d->iconName.isEmpty()) {
        // Make default icon name from the mimetype name
        d->iconName = name();
        const int slashindex = d->iconName.indexOf(QLatin1Char('/'));
        if (slashindex != -1)
            d->iconName[slashindex] = QLatin1Char('-');
    }
    return d->iconName;
}

/*!
    \fn QStringList QMimeType::globPatterns() const;
    Returns the list of glob matching patterns.
 */
QStringList QMimeType::globPatterns() const
{
    QMimeDatabasePrivate::instance()->provider()->loadMimeTypePrivate(*d);
    return d->globPatterns;
}

/*!
    A type is a subclass of another type if any instance of the first type is
    also an instance of the second. For example, all image/svg+xml files are also
    text/xml, text/plain and application/octet-stream files. Subclassing is about
    the format, rather than the category of the data (for example, there is no
    'generic spreadsheet' class that all spreadsheets inherit from).
    Conversely, the parent mimetype of image/svg+xml is text/xml.

    A mimetype can have multiple parents. For instance application/x-perl
    has two parents: application/x-executable and text/plain. This makes
    it possible to both execute perl scripts, and to open them in text editors.
*/
QStringList QMimeType::parentMimeTypes() const
{
    return QMimeDatabasePrivate::instance()->provider()->parents(d->name);
}

static void collectParentMimeTypes(const QString &mime, QStringList &allParents)
{
    QStringList parents = QMimeDatabasePrivate::instance()->provider()->parents(mime);
    foreach (const QString &parent, parents) {
        // I would use QSet, but since order matters I better not
        if (!allParents.contains(parent))
            allParents.append(parent);
    }
    // We want a breadth-first search, so that the least-specific parent (octet-stream) is last
    // This means iterating twice, unfortunately.
    foreach (const QString &parent, parents) {
        collectParentMimeTypes(parent, allParents);
    }
}

/*!
    Return all the parent mimetypes of this mimetype, direct and indirect.
    This includes the parent(s) of its parent(s), etc.

    For instance, for image/svg+xml the list would be:
    application/xml, text/plain, application/octet-stream.

    Note that application/octet-stream is the ultimate parent for all types
    of files (but not directories).
*/
QStringList QMimeType::allAncestors() const
{
    QStringList allParents;
    collectParentMimeTypes(d->name, allParents);
    return allParents;
}

/*!
    Return the list of aliases of this mimetype.

    For instance, for text/csv, the returned list would be:
    text/x-csv, text/x-comma-separated-values.

    Note that all QMimeType instances refer to proper mimetypes,
    never to aliases directly.

    The order of the aliases in the list is undefined.
*/
QStringList QMimeType::aliases() const
{
    return QMimeDatabasePrivate::instance()->provider()->listAliases(d->name);
}

/*!
    Returns the known suffixes for the MIME type.
    No leading dot is included, so for instance this would return "jpg", "jpeg" for image/jpeg.
 */
QStringList QMimeType::suffixes() const
{
    QMimeDatabasePrivate::instance()->provider()->loadMimeTypePrivate(*d);

    QStringList result;
    foreach (const QString &pattern, d->globPatterns) {
        // Not a simple suffix if it looks like: README or *. or *.* or *.JP*G or *.JP?
        if (pattern.startsWith(QLatin1String("*.")) &&
            pattern.length() > 2 &&
            pattern.indexOf(QLatin1Char('*'), 2) < 0 && pattern.indexOf(QLatin1Char('?'), 2) < 0) {
            const QString suffix = pattern.mid(2);
            result.append(suffix);
        }
    }

    return result;
}

/*!
    Returns the preferred suffix for the MIME type.
    No leading dot is included, so for instance this would return "pdf" for application/pdf.
    The return value can be empty, for mime types which do not have any suffixes associated.
 */
QString QMimeType::preferredSuffix() const
{
    const QStringList suffixList = suffixes();
    return suffixList.isEmpty() ? QString() : suffixList.at(0);
}

/*!
    \fn QString QMimeType::filterString() const;
    Returns a filter string usable for a file dialog.
*/
QString QMimeType::filterString() const
{
    QMimeDatabasePrivate::instance()->provider()->loadMimeTypePrivate(*d);
    QString filter;

    if (!d->globPatterns.empty()) {
        filter += comment() + QLatin1String(" (");
        for (int i = 0; i < d->globPatterns.size(); ++i) {
            if (i != 0)
                filter += QLatin1Char(' ');
            filter += d->globPatterns.at(i);
        }
        filter +=  QLatin1Char(')');
    }

    return filter;
}

/*!
    \fn bool QMimeType::inherits(const QString &mimeTypeName) const;
    Returns \c true if this mimetype is \a mimeTypeName,
    or inherits \a mimeTypeName (see parentMimeTypes()),
    or \a mimeTypeName is an alias for this mimetype.
 */
bool QMimeType::inherits(const QString &mimeTypeName) const
{
    if (d->name == mimeTypeName)
        return true;
    return QMimeDatabasePrivate::instance()->inherits(d->name, mimeTypeName);
}

QT_END_NAMESPACE
