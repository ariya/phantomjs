/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "qwebpluginfactory.h"

/*!
    \class QWebPluginFactory
    \since 4.4
    \brief The QWebPluginFactory class is used to embed custom data types in web pages.

    \inmodule QtWebKit

    The HTML \c{<object>} tag is used to embed arbitrary content into a web page,
    for example:

    \code
    <object type="application/x-pdf" data="http://qt.nokia.com/document.pdf" width="500" height="400"></object>
    \endcode

    QtWebkit will natively handle the most basic data types like \c{text/html} and
    \c{image/jpeg}, but for any advanced or custom data types you will need to
    provide a handler yourself.

    QWebPluginFactory is a factory for creating plugins for QWebPage, where each
    plugin provides support for one or more data types. A plugin factory can be
    installed on a QWebPage using QWebPage::setPluginFactory().

    \note The plugin factory is only used if plugins are enabled through QWebSettings.

    You provide a QWebPluginFactory by implementing the plugins() and the
    create() methods. For plugins() it is necessary to describe the plugins the
    factory can create, including a description and the supported MIME types.
    The MIME types each plugin can handle should match the ones specified in
    in the HTML \c{<object>} tag of your content.

    The create() method is called if the requested MIME type is supported. The
    implementation has to return a new instance of the plugin requested for the
    given MIME type and the specified URL.

    The plugins themselves are subclasses of QObject, but currently only plugins
    based on either QWidget or QGraphicsWidget are supported.

*/


/*!
    \class QWebPluginFactory::Plugin
    \since 4.4
    \brief The QWebPluginFactory::Plugin structure describes the properties of a plugin a QWebPluginFactory can create.

    \inmodule QtWebKit
*/

/*!
    \variable QWebPluginFactory::Plugin::name
    The name of the plugin.
*/

/*!
    \variable QWebPluginFactory::Plugin::description
    The description of the plugin.
*/

/*!
    \variable QWebPluginFactory::Plugin::mimeTypes
    The list of mime types supported by the plugin.
*/

/*!
    \class QWebPluginFactory::MimeType
    \since 4.4
    \brief The QWebPluginFactory::MimeType structure describes a mime type supported by a plugin.

    \inmodule QtWebKit
*/

/*!
    Returns true if this mimetype is the same as the \a other mime type.
*/
bool QWebPluginFactory::MimeType::operator==(const MimeType& other) const
{
    return name == other.name
           && description == other.description
           && fileExtensions == other.fileExtensions;
}

/*!
    \fn bool QWebPluginFactory::MimeType::operator!=(const MimeType& other) const

    Returns true if this mimetype is different from the \a other mime type.
*/

/*!
    \variable QWebPluginFactory::MimeType::name

    The full name of the MIME type; e.g., \c{text/plain} or \c{image/png}.
*/

/*!
    \variable QWebPluginFactory::MimeType::description
    The description of the mime type.
*/

/*!
    \variable QWebPluginFactory::MimeType::fileExtensions
    The list of file extensions that are used by this mime type.

    For example, a mime type for PDF documents would return "pdf" as its file extension.
*/

/*!
    Constructs a QWebPluginFactory with parent \a parent.
*/
QWebPluginFactory::QWebPluginFactory(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destructor.
*/
QWebPluginFactory::~QWebPluginFactory()
{
}

/*!
    \fn QList<Plugin> QWebPluginFactory::plugins() const = 0

    This function is reimplemented in subclasses to return a list of
    supported plugins the factory can create.

    \note Currently, this function is only called when JavaScript programs
    access the global \c plugins or \c mimetypes objects.
*/

/*!
    This function is called to refresh the list of supported plugins. It may be called after a new plugin
    has been installed in the system.
*/
void QWebPluginFactory::refreshPlugins()
{
}

/*!
    \fn QObject *QWebPluginFactory::create(const QString &mimeType, const QUrl &url,
    const QStringList &argumentNames, const QStringList &argumentValues) const = 0

    Implemented in subclasses to create a new plugin that can display content of
    the MIME type given by \a mimeType. The URL of the content is provided in \a url.
    The returned object should be a QWidget.

    The HTML object element can provide parameters through the \c{<param>} tag.
    The name and the value attributes of these tags are specified by the
    \a argumentNames and \a argumentValues string lists.

    For example:

    \code
    <object type="application/x-pdf" data="http://qt.nokia.com/document.pdf" width="500" height="400">
        <param name="showTableOfContents" value="true" />
        <param name="hideThumbnails" value="false" />
    </object>
    \endcode

    The above object element will result in a call to create() with the following arguments:
    \table
    \header \o Parameter
            \o Value
    \row    \o mimeType
            \o "application/x-pdf"
    \row    \o url
            \o "http://qt.nokia.com/document.pdf"
    \row    \o argumentNames
            \o "showTableOfContents" "hideThumbnails"
    \row    \o argumentVaues
            \o "true" "false"
    \endtable

    \note Ownership of the returned object will be transferred to the caller.
*/

/*!
    \enum QWebPluginFactory::Extension
    \internal

    This enum describes the types of extensions that the plugin factory can support. Before using these extensions, you
    should verify that the extension is supported by calling supportsExtension().

    Currently there are no extensions.
*/

/*!
    \class QWebPluginFactory::ExtensionOption
    \internal
    \since 4.4
    \brief The ExtensionOption class provides an extended input argument to QWebPluginFactory's extension support.

    \inmodule QtWebKit

    \sa QWebPluginFactory::extension()
*/

/*!
    \class QWebPluginFactory::ExtensionReturn
    \internal
    \since 4.4
    \brief The ExtensionOption class provides an extended output argument to QWebPluginFactory's extension support.

    \inmodule QtWebKit

    \sa QWebPluginFactory::extension()
*/

/*!
    This virtual function can be reimplemented in a QWebPluginFactory subclass to provide support for extensions. The \a option
    argument is provided as input to the extension; the output results can be stored in \a output.

    \internal

    The behaviour of this function is determined by \a extension.

    You can call supportsExtension() to check if an extension is supported by the factory.

    By default, no extensions are supported, and this function returns false.

    \sa supportsExtension(), Extension
*/
bool QWebPluginFactory::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
    Q_UNUSED(extension)
    Q_UNUSED(option)
    Q_UNUSED(output)
    return false;
}

/*!
    This virtual function returns true if the plugin factory supports \a extension; otherwise false is returned.

    \internal

    \sa extension()
*/
bool QWebPluginFactory::supportsExtension(Extension extension) const
{
    Q_UNUSED(extension)
    return false;
}
