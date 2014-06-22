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

#include "qloggingcategory.h"
#include "qloggingcategory_p.h"
#include "qloggingregistry_p.h"

QT_BEGIN_NAMESPACE

const char qtDefaultCategoryName[] = "default";

Q_GLOBAL_STATIC_WITH_ARGS(QLoggingCategory, qtDefaultCategory,
                          (qtDefaultCategoryName))

/*!
    \class QLoggingCategory
    \inmodule QtCore
    \since 5.2

    \brief The QLoggingCategory class represents a category, or 'area' in the
    logging infrastructure.

    QLoggingCategory represents a certain logging category - identified
    by a string - at runtime. Whether a category should be actually logged or
    not can be checked with the \l isEnabled() methods.

    All objects are meant to be configured by a common registry (see also
    \l{Configuring Categories}). Different objects can also represent the same
    category. It's therefore not recommended to export objects across module
    boundaries, nor to manipulate the objects directly, nor to inherit from
    QLoggingCategory.

    \section1 Creating category objects

    The Q_LOGGING_CATEGORY() and the Q_DECLARE_LOGGING_CATEGORY() macros
    conveniently declare and create QLoggingCategory objects:

    \snippet qloggingcategory/main.cpp 1

    \section1 Checking category configuration

    QLoggingCategory provides \l isDebugEnabled(), \l isWarningEnabled(),
    \l isCriticalEnabled(), as well as \l isEnabled()
    to check whether messages for the given message type should be logged.

    \note The qCDebug(), qCWarning(), qCCritical() macros prevent arguments
    from being evaluated if the respective message types are not enabled for the
    category, so explicit checking is not needed:

    \snippet qloggingcategory/main.cpp 4

    \section1 Default category configuration

    In the default configuration \l isWarningEnabled() , \l isDebugEnabled() and
    \l isCriticalEnabled() will return \c true.

    \section1 Configuring Categories

    Categories can be centrally configured by either setting logging rules,
    or by installing a custom filter.

    \section2 Logging Rules

    Logging rules allow to enable or disable logging for categories in a flexible
    way. Rules are specified in text, where every line must have the format

    \code
    <category>[.<type>] = true|false
    \endcode

    \c <category> is the name of the category, potentially with \c{*} as a
    wildcard symbol as the first or last character (or at both positions).
    The optional \c <type> must be either \c debug, \c warning, or \c critical.
    Lines that do not fit to his scheme are ignored.

    Rules are evaluated in text order, from first to last. That is, if two rules
    apply to a category/type, the rule that comes later is applied.

    Rules can be set via \l setFilterRules(). Since Qt 5.3 logging rules can also
    be set in the \c QT_LOGGING_RULES environment variable, and
    are automatically loaded from the \c [Rules] section of a logging
    configuration file. Such configuration files are looked up in the QtProject
    configuration directory, or explicitly set in a \c QT_LOGGING_CONF
    environment variable.

    Rules set by \l setFilterRules() take precedence over rules specified
    in the QtProject configuration directory, and can, in turn, be
    overwritten by rules from the configuration file specified by
    \c QT_LOGGING_CONF, and rules set by \c QT_LOGGING_RULES.


    Order of evaluation:
    \list
    \li Rules from QtProject/qlogging.ini
    \li Rules set by \l setFilterRules()
    \li Rules from file in \c QT_LOGGING_CONF
    \li Rules from environment variable QT_LOGGING_RULES
    \endlist

    The \c QtProject/qlogging.ini file is looked up in all directories returned
    by QStandardPaths::GenericConfigLocation, e.g.

    \list
    \li on Mac OS X: \c ~/Library/Preferences
    \li on Unix: \c ~/.config, \c /etc/xdg
    \li on Windows: \c %LOCALAPPDATA%, \c %ProgramData%,
        \l QCoreApplication::applicationDirPath(),
        QCoreApplication::applicationDirPath() + \c "/data"
    \endlist

    Set the \c QT_LOGGING_DEBUG environment variable to see from where
    logging rules are loaded.

    \section2 Installing a Custom Filter

    As a lower-level alternative to the text rules you can also implement a
    custom filter via \l installFilter(). All filter rules are ignored in this
    case.

    \section1 Printing the category

    Use the \c %{category} place holder to print the category in the default
    message handler:

    \snippet qloggingcategory/main.cpp 3
*/

/*!
    Constructs a QLoggingCategory object with the provided \a category name.
    The object becomes the local identifier for the category.

    If \a category is \c{0}, the category name is changed to \c "default".
*/
QLoggingCategory::QLoggingCategory(const char *category)
    : d(0),
      name(0),
      enabledDebug(true),
      enabledWarning(true),
      enabledCritical(true)
{
    Q_UNUSED(d);
    Q_UNUSED(placeholder);

    const bool isDefaultCategory
            = (category == 0) || (strcmp(category, qtDefaultCategoryName) == 0);

    // normalize "default" category name, so that we can just do
    // pointer comparison in QLoggingRegistry::updateCategory
    if (isDefaultCategory) {
        name = qtDefaultCategoryName;
    } else {
        name = category;
    }

    if (QLoggingRegistry *reg = QLoggingRegistry::instance())
        reg->registerCategory(this);
}

/*!
    Destructs a QLoggingCategory object.
*/
QLoggingCategory::~QLoggingCategory()
{
    if (QLoggingRegistry *reg = QLoggingRegistry::instance())
        reg->unregisterCategory(this);
}

/*!
   \fn const char *QLoggingCategory::categoryName() const

    Returns the name of the category.
*/

/*!
    \fn bool QLoggingCategory::isDebugEnabled() const

    Returns \c true if debug messages should be shown for this category.
    Returns \c false otherwise.

    \note The \l qCDebug() macro already does this check before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/

/*!
    \fn bool QLoggingCategory::isWarningEnabled() const

    Returns \c true if warning messages should be shown for this category.
    Returns \c false otherwise.

    \note The \l qCWarning() macro already does this check before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/

/*!
    \fn bool QLoggingCategory::isCriticalEnabled() const

    Returns \c true if critical messages should be shown for this category.
    Returns \c false otherwise.

    \note The \l qCCritical() macro already does this check before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/

/*!
    Returns \c true if a message of type \a msgtype for the category should be
    shown. Returns \c false otherwise.
*/
bool QLoggingCategory::isEnabled(QtMsgType msgtype) const
{
    switch (msgtype) {
    case QtDebugMsg: return enabledDebug;
    case QtWarningMsg: return enabledWarning;
    case QtCriticalMsg: return enabledCritical;
    case QtFatalMsg: return true;
    }
    return false;
}

/*!
    Changes the message type \a type for the category to \a enable.

    \note Changes only affect the current QLoggingCategory object, and won't
    change the settings of other objects for the same category name.
    Use either \l setFilterRules() or \l installFilter() to change the
    configuration globally.

    \note \c QtFatalMsg cannot be changed. It will always return \c true.
*/
void QLoggingCategory::setEnabled(QtMsgType type, bool enable)
{
    switch (type) {
    case QtDebugMsg: enabledDebug = enable; break;
    case QtWarningMsg: enabledWarning = enable; break;
    case QtCriticalMsg: enabledCritical = enable; break;
    case QtFatalMsg: break;
    }
}

/*!
    \fn QLoggingCategory &QLoggingCategory::operator()()

    Returns the object itself. This allows both a QLoggingCategory variable, and
    a factory method returning a QLoggingCategory, to be used in \l qCDebug(),
    \l qCWarning(), \l qCCritical() macros.
 */

/*!
    \fn const QLoggingCategory &QLoggingCategory::operator()() const

    Returns the object itself. This allows both a QLoggingCategory variable, and
    a factory method returning a QLoggingCategory, to be used in \l qCDebug(),
    \l qCWarning(), \l qCCritical() macros.
 */

/*!
    Returns a pointer to the global category \c "default" that
    is used e.g. by qDebug(), qWarning(), qCritical(), qFatal().

    \note The returned pointer may be null during destruction of
    static objects.

    \note Ownership of the category is not transferred, do not
    \c delete the returned pointer.

 */
QLoggingCategory *QLoggingCategory::defaultCategory()
{
    return qtDefaultCategory();
}

/*!
    \typedef QLoggingCategory::CategoryFilter

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet qloggingcategory/main.cpp 20

    A function with this signature can be installed with \l installFilter().
*/

/*!
    Installs a function \a filter that is used to determine which categories
    and message types should be enabled. Returns a pointer to the previous
    installed filter.

    Every QLoggingCategory object created is passed to the filter, and the
    filter is free to change the respective category configuration with
    \l setEnabled().

    The filter might be called concurrently from different threads, and
    therefore has to be reentrant.

    Example:
    \snippet qloggingcategory/main.cpp 21

    An alternative way of configuring the default filter is via
    \l setFilterRules().
 */
QLoggingCategory::CategoryFilter
QLoggingCategory::installFilter(QLoggingCategory::CategoryFilter filter)
{
    return QLoggingRegistry::instance()->installFilter(filter);
}

/*!
    Configures which categories and message types should be enabled through a
    a set of \a rules.

    Example:

    \snippet qloggingcategory/main.cpp 2

    \note The rules might be ignored if a custom category filter is installed
    with \l installFilter(), or if the user defined \c QT_LOGGING_CONF or \c QT_LOGGING_RULES
    environment variable.
*/
void QLoggingCategory::setFilterRules(const QString &rules)
{
    QLoggingRegistry::instance()->setApiRules(rules);
}

/*!
    \macro qCDebug(category)
    \relates QLoggingCategory
    \since 5.2

    Returns an output stream for debug messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isDebugEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 10

    \note Arguments are not processed if debug output for the category is not
    enabled, so do not rely on any side effects.

    \sa qDebug()
*/

/*!
    \macro qCDebug(category, const char *message, ...)
    \relates QLoggingCategory
    \since 5.3

    Logs a debug message \a message in the logging category \a category.
    \a message might contain place holders that are replaced by additional
    arguments, similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp 13

    \note Arguments might not be processed if debug output for the category is
    not enabled, so do not rely on any side effects.

    \sa qDebug()
*/

/*!
    \macro qCWarning(category)
    \relates QLoggingCategory
    \since 5.2

    Returns an output stream for warning messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isWarningEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 11

    \note Arguments are not processed if warning output for the category is not
    enabled, so do not rely on any side effects.

    \sa qWarning()
*/

/*!
    \macro qCWarning(category, const char *message, ...)
    \relates QLoggingCategory
    \since 5.3

    Logs a warning message \a message in the logging category \a category.
    \a message might contain place holders that are replaced by additional
    arguments, similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp 14

    \note Arguments might not be processed if warning output for the category is
    not enabled, so do not rely on any side effects.

    \sa qWarning()
*/

/*!
    \macro qCCritical(category)
    \relates QLoggingCategory
    \since 5.2

    Returns an output stream for critical messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isCriticalEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 12

    \note Arguments are not processed if critical output for the category is not
    enabled, so do not rely on any side effects.

    \sa qCritical()
*/

/*!
    \macro qCCritical(category, const char *message, ...)
    \relates QLoggingCategory
    \since 5.3

    Logs a critical message \a message in the logging category \a category.
    \a message might contain place holders that are replaced by additional
    arguments, similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp 15

    \note Arguments might not be processed if critical output for the category
    is not enabled, so do not rely on any side effects.

    \sa qCritical()
*/
/*!
    \macro Q_DECLARE_LOGGING_CATEGORY(name)
    \relates QLoggingCategory
    \since 5.2

    Declares a logging category \a name. The macro can be used to declare
    a common logging category shared in different parts of the program.

    This macro must be used outside of a class or method.
*/

/*!
    \macro Q_LOGGING_CATEGORY(name, string)
    \relates QLoggingCategory
    \since 5.2

    Defines a logging category \a name, and makes it configurable under the
    \a string identifier.

    Only one translation unit in a library or executable can define a category
    with a specific name.

    This macro must be used outside of a class or method.
*/

QT_END_NAMESPACE
