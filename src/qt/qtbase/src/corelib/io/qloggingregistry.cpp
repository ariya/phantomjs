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

#include "qloggingregistry_p.h"
#include "qloggingcategory_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qdir.h>

// We can't use the default macros because this would lead to recursion.
// Instead let's define our own one that unconditionally logs...
#define debugMsg QMessageLogger(__FILE__, __LINE__, __FUNCTION__, "qt.core.logging").debug
#define warnMsg QMessageLogger(__FILE__, __LINE__, __FUNCTION__, "qt.core.logging").warning


QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QLoggingRegistry, qtLoggingRegistry)

/*!
    \internal
    Constructs a logging rule with default values.
*/
QLoggingRule::QLoggingRule() :
    enabled(false)
{
}

/*!
    \internal
    Constructs a logging rule.
*/
QLoggingRule::QLoggingRule(const QStringRef &pattern, bool enabled) :
    messageType(-1),
    enabled(enabled)
{
    parse(pattern);
}

/*!
    \internal
    Return value 1 means filter passed, 0 means filter doesn't influence this
    category, -1 means category doesn't pass this filter.
 */
int QLoggingRule::pass(const QString &cat, QtMsgType msgType) const
{
    // check message type
    if (messageType > -1 && messageType != msgType)
        return 0;

    if (flags == FullText) {
        // full match
        if (category == cat)
            return (enabled ? 1 : -1);
        else
            return 0;
    }

    const int idx = cat.indexOf(category);
    if (idx >= 0) {
        if (flags == MidFilter) {
            // matches somewhere
            if (idx >= 0)
                return (enabled ? 1 : -1);
        } else if (flags == LeftFilter) {
            // matches left
            if (idx == 0)
                return (enabled ? 1 : -1);
        } else if (flags == RightFilter) {
            // matches right
            if (idx == (cat.count() - category.count()))
                return (enabled ? 1 : -1);
        }
    }
    return 0;
}

/*!
    \internal
    Parses \a pattern.
    Allowed is f.ex.:
             qt.core.io.debug      FullText, QtDebugMsg
             qt.core.*             LeftFilter, all types
             *.io.warning          RightFilter, QtWarningMsg
             *.core.*              MidFilter
 */
void QLoggingRule::parse(const QStringRef &pattern)
{
    QStringRef p;

    // strip trailing ".messagetype"
    if (pattern.endsWith(QLatin1String(".debug"))) {
        p = QStringRef(pattern.string(), pattern.position(),
                       pattern.length() - strlen(".debug"));
        messageType = QtDebugMsg;
    } else if (pattern.endsWith(QLatin1String(".warning"))) {
        p = QStringRef(pattern.string(), pattern.position(),
                       pattern.length() - strlen(".warning"));
        messageType = QtWarningMsg;
    } else if (pattern.endsWith(QLatin1String(".critical"))) {
        p = QStringRef(pattern.string(), pattern.position(),
                       pattern.length() - strlen(".critical"));
        messageType = QtCriticalMsg;
    } else {
        p = pattern;
    }

    if (!p.contains(QLatin1Char('*'))) {
        flags = FullText;
    } else {
        if (p.endsWith(QLatin1Char('*'))) {
            flags |= LeftFilter;
            p = QStringRef(p.string(), p.position(), p.length() - 1);
        }
        if (p.startsWith(QLatin1Char('*'))) {
            flags |= RightFilter;
            p = QStringRef(p.string(), p.position() + 1, p.length() - 1);
        }
        if (p.contains(QLatin1Char('*'))) // '*' only supported at start/end
            flags = 0;
    }

    category = p.toString();
}

/*!
    \class QLoggingSettingsParser
    \since 5.3
    \internal

    Parses a .ini file with the following format:

    [rules]
    rule1=[true|false]
    rule2=[true|false]
    ...

    [rules] is the default section, and therefore optional.
*/

/*!
    \internal
    Parses configuration from \a content.
*/
void QLoggingSettingsParser::setContent(const QString &content)
{
    QString content_ = content;
    QTextStream stream(&content_, QIODevice::ReadOnly);
    setContent(stream);
}

/*!
    \internal
    Parses configuration from \a stream.
*/
void QLoggingSettingsParser::setContent(QTextStream &stream)
{
    _rules.clear();
    while (!stream.atEnd()) {
        QString line = stream.readLine();

        // Remove all whitespace from line
        line = line.simplified();
        line.remove(QLatin1Char(' '));

        // comment
        if (line.startsWith(QLatin1Char(';')))
            continue;

        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            // new section
            _section = line.mid(1, line.size() - 2);
            continue;
        }

        if (_section == QLatin1String("Rules")) {
            int equalPos = line.indexOf(QLatin1Char('='));
            if ((equalPos != -1)
                    && (line.lastIndexOf(QLatin1Char('=')) == equalPos)) {
                const QStringRef pattern = line.leftRef(equalPos);
                const QStringRef valueStr = line.midRef(equalPos + 1);
                int value = -1;
                if (valueStr == QLatin1String("true"))
                    value = 1;
                else if (valueStr == QLatin1String("false"))
                    value = 0;
                QLoggingRule rule(pattern, (value == 1));
                if (rule.flags != 0 && (value != -1))
                    _rules.append(rule);
                else
                    warnMsg("Ignoring malformed logging rule: '%s'", line.toUtf8().constData());
            }
        }
    }
}

/*!
    \internal
    QLoggingRegistry constructor
 */
QLoggingRegistry::QLoggingRegistry()
    : categoryFilter(defaultCategoryFilter)
{
}

static bool qtLoggingDebug()
{
    static const bool debugEnv = qEnvironmentVariableIsSet("QT_LOGGING_DEBUG");
    return debugEnv;
}

/*!
    \internal
    Initializes the rules database by loading
    $QT_LOGGING_CONF, $QT_LOGGING_RULES, and .config/QtProject/qtlogging.ini.
 */
void QLoggingRegistry::init()
{
    // get rules from environment
    const QByteArray rulesFilePath = qgetenv("QT_LOGGING_CONF");
    if (!rulesFilePath.isEmpty()) {
        QFile file(QFile::decodeName(rulesFilePath));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QLoggingSettingsParser parser;
            parser.setContent(stream);
            if (qtLoggingDebug())
                debugMsg("Loading \"%s\" ...",
                         QDir::toNativeSeparators(file.fileName()).toUtf8().constData());
            envRules = parser.rules();
        }
    }
    const QByteArray rulesSrc = qgetenv("QT_LOGGING_RULES");
    if (!rulesSrc.isEmpty()) {
         QTextStream stream(rulesSrc);
         QLoggingSettingsParser parser;
         parser.setSection(QStringLiteral("Rules"));
         parser.setContent(stream);
         envRules += parser.rules();
    }

    // get rules from qt configuration
    QString envPath = QStandardPaths::locate(QStandardPaths::GenericConfigLocation,
                                             QStringLiteral("QtProject/qtlogging.ini"));
    if (!envPath.isEmpty()) {
        QFile file(envPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QLoggingSettingsParser parser;
            parser.setContent(stream);
            if (qtLoggingDebug())
                debugMsg("Loading \"%s\" ...",
                         QDir::toNativeSeparators(envPath).toUtf8().constData());
            configRules = parser.rules();
        }
    }

    if (!envRules.isEmpty() || !configRules.isEmpty()) {
        QMutexLocker locker(&registryMutex);
        updateRules();
    }
}

/*!
    \internal
    Registers a category object.

    This method might be called concurrently for the same category object.
*/
void QLoggingRegistry::registerCategory(QLoggingCategory *cat)
{
    QMutexLocker locker(&registryMutex);

    if (!categories.contains(cat)) {
        categories.append(cat);
        (*categoryFilter)(cat);
    }
}

/*!
    \internal
    Unregisters a category object.
*/
void QLoggingRegistry::unregisterCategory(QLoggingCategory *cat)
{
    QMutexLocker locker(&registryMutex);

    categories.removeOne(cat);
}

/*!
    \internal
    Installs logging rules as specified in \a content.
 */
void QLoggingRegistry::setApiRules(const QString &content)
{
    QLoggingSettingsParser parser;
    parser.setSection(QStringLiteral("Rules"));
    parser.setContent(content);

    QMutexLocker locker(&registryMutex);

    if (qtLoggingDebug())
        debugMsg("Loading logging rules set by Qt API ...");

    apiRules = parser.rules();

    updateRules();
}

/*!
    \internal
    Activates a new set of logging rules for the default filter.

    (The caller must lock registryMutex to make sure the API is thread safe.)
*/
void QLoggingRegistry::updateRules()
{
    if (categoryFilter != defaultCategoryFilter)
        return;

    rules = configRules + apiRules + envRules;

    foreach (QLoggingCategory *cat, categories)
        (*categoryFilter)(cat);
}

/*!
    \internal
    Installs a custom filter rule.
*/
QLoggingCategory::CategoryFilter
QLoggingRegistry::installFilter(QLoggingCategory::CategoryFilter filter)
{
    QMutexLocker locker(&registryMutex);

    if (filter == 0)
        filter = defaultCategoryFilter;

    QLoggingCategory::CategoryFilter old = categoryFilter;
    categoryFilter = filter;

    foreach (QLoggingCategory *cat, categories)
        (*categoryFilter)(cat);

    return old;
}

QLoggingRegistry *QLoggingRegistry::instance()
{
    return qtLoggingRegistry();
}

/*!
    \internal
    Updates category settings according to rules.
*/
void QLoggingRegistry::defaultCategoryFilter(QLoggingCategory *cat)
{
    // QLoggingCategory() normalizes "default" strings
    // to qtDefaultCategoryName
    bool debug = true;
    char c;
    if (!memcmp(cat->categoryName(), "qt", 2) && (!(c = cat->categoryName()[2]) || c == '.'))
        debug = false;

    bool warning = true;
    bool critical = true;

    QString categoryName = QLatin1String(cat->categoryName());
    QLoggingRegistry *reg = QLoggingRegistry::instance();
    foreach (const QLoggingRule &item, reg->rules) {
        int filterpass = item.pass(categoryName, QtDebugMsg);
        if (filterpass != 0)
            debug = (filterpass > 0);
        filterpass = item.pass(categoryName, QtWarningMsg);
        if (filterpass != 0)
            warning = (filterpass > 0);
        filterpass = item.pass(categoryName, QtCriticalMsg);
        if (filterpass != 0)
            critical = (filterpass > 0);
    }

    cat->setEnabled(QtDebugMsg, debug);
    cat->setEnabled(QtWarningMsg, warning);
    cat->setEnabled(QtCriticalMsg, critical);
}


QT_END_NAMESPACE
