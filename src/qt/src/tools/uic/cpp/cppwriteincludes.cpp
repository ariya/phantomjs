/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "cppwriteincludes.h"
#include "driver.h"
#include "ui4.h"
#include "uic.h"
#include "databaseinfo.h"

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

#include <stdio.h>

QT_BEGIN_NAMESPACE

enum { debugWriteIncludes = 0 };
enum { warnHeaderGeneration = 0 };

struct ClassInfoEntry
{
    const char *klass;
    const char *module;
    const char *header;
};

static const ClassInfoEntry qclass_lib_map[] = {
#define QT_CLASS_LIB(klass, module, header) { #klass, #module, #header },
#include "qclass_lib_map.h"

#undef QT_CLASS_LIB
};

// Format a module header as 'QtCore/QObject'
static inline QString moduleHeader(const QString &module, const QString &header)
{
    QString rc = module;
    rc += QLatin1Char('/');
    rc += header;
    return rc;
}

namespace CPP {

WriteIncludes::WriteIncludes(Uic *uic)
    : m_uic(uic), m_output(uic->output()), m_scriptsActivated(false), m_laidOut(false)
{
    // When possible (no namespace) use the "QtModule/QClass" convention
    // and create a re-mapping of the old header "qclass.h" to it. Do not do this
    // for the "Phonon::Someclass" classes, however.
    const QString namespaceDelimiter = QLatin1String("::");
    const ClassInfoEntry *classLibEnd = qclass_lib_map + sizeof(qclass_lib_map)/sizeof(ClassInfoEntry);    
    for(const ClassInfoEntry *it = qclass_lib_map; it < classLibEnd;  ++it) {        
        const QString klass = QLatin1String(it->klass);
        const QString module = QLatin1String(it->module);
        QLatin1String header = QLatin1String(it->header);
        if (klass.contains(namespaceDelimiter)) {
            m_classToHeader.insert(klass, moduleHeader(module, header));
        } else {
            const QString newHeader = moduleHeader(module, klass);
            m_classToHeader.insert(klass, newHeader);
            m_oldHeaderToNewHeader.insert(header, newHeader);
        }
    }
}

void WriteIncludes::acceptUI(DomUI *node)
{
    m_scriptsActivated = false;
    m_laidOut = false;
    m_localIncludes.clear();
    m_globalIncludes.clear();
    m_knownClasses.clear();
    m_includeBaseNames.clear();

    if (node->elementIncludes())
        acceptIncludes(node->elementIncludes());

    if (node->elementCustomWidgets())
        TreeWalker::acceptCustomWidgets(node->elementCustomWidgets());

    add(QLatin1String("QApplication"));
    add(QLatin1String("QVariant"));
    add(QLatin1String("QAction"));

    add(QLatin1String("QButtonGroup")); // ### only if it is really necessary
    add(QLatin1String("QHeaderView"));

    if (m_uic->hasExternalPixmap() && m_uic->pixmapFunction() == QLatin1String("qPixmapFromMimeSource")) {
#ifdef QT_NO_QT3_SUPPORT
        qWarning("%s: Warning: The form file has external pixmaps or qPixmapFromMimeSource() set as a pixmap function. "
                 "This requires Qt 3 support, which is disabled. The resulting code will not compile.",
                 qPrintable(m_uic->option().messagePrefix()));
#endif
        add(QLatin1String("Q3MimeSourceFactory"));
    }

    if (m_uic->databaseInfo()->connections().size()) {
        add(QLatin1String("QSqlDatabase"));
        add(QLatin1String("Q3SqlCursor"));
        add(QLatin1String("QSqlRecord"));
        add(QLatin1String("Q3SqlForm"));
    }

    TreeWalker::acceptUI(node);

    writeHeaders(m_globalIncludes, true);
    writeHeaders(m_localIncludes, false);

    m_output << QLatin1Char('\n');
}

void WriteIncludes::acceptWidget(DomWidget *node)
{
    if (debugWriteIncludes)
        fprintf(stderr, "%s '%s'\n", Q_FUNC_INFO, qPrintable(node->attributeClass()));

    add(node->attributeClass());
    TreeWalker::acceptWidget(node);
}

void WriteIncludes::acceptLayout(DomLayout *node)
{
    add(node->attributeClass());
    m_laidOut = true;
    TreeWalker::acceptLayout(node);
}

void WriteIncludes::acceptSpacer(DomSpacer *node)
{
    add(QLatin1String("QSpacerItem"));
    TreeWalker::acceptSpacer(node);
}

void WriteIncludes::acceptProperty(DomProperty *node)
{
    if (node->kind() == DomProperty::Date)
        add(QLatin1String("QDate"));
    if (node->kind() == DomProperty::Locale)
        add(QLatin1String("QLocale"));
    TreeWalker::acceptProperty(node);
}

void WriteIncludes::insertIncludeForClass(const QString &className, QString header, bool global)
{
    if (debugWriteIncludes)
        fprintf(stderr, "%s %s '%s' %d\n", Q_FUNC_INFO, qPrintable(className), qPrintable(header), global);

    do {
        if (!header.isEmpty())
            break;

        // Known class        
        const StringMap::const_iterator it = m_classToHeader.constFind(className);
        if (it != m_classToHeader.constEnd()) {
            header = it.value();
            global =  true;
            break;
        }

        // Quick check by class name to detect includehints provided for custom widgets.
        // Remove namespaces
        QString lowerClassName = className.toLower();
        static const QString namespaceSeparator = QLatin1String("::");
        const int namespaceIndex = lowerClassName.lastIndexOf(namespaceSeparator);
        if (namespaceIndex != -1)
            lowerClassName.remove(0, namespaceIndex + namespaceSeparator.size());
        if (m_includeBaseNames.contains(lowerClassName)) {
            header.clear();
            break;
        }

        // Last resort: Create default header
        if (!m_uic->option().implicitIncludes)
            break;
        header = lowerClassName;
        header += QLatin1String(".h");
        if (warnHeaderGeneration) {
            qWarning("%s: Warning: generated header '%s' for class '%s'.",
                     qPrintable(m_uic->option().messagePrefix()),
                     qPrintable(header), qPrintable(className));

        }

        global = true;
    } while (false);

    if (!header.isEmpty())
        insertInclude(header, global);
}

void WriteIncludes::add(const QString &className, bool determineHeader, const QString &header, bool global)
{
    if (debugWriteIncludes)
            fprintf(stderr, "%s %s '%s' %d\n", Q_FUNC_INFO, qPrintable(className), qPrintable(header), global);

    if (className.isEmpty() || m_knownClasses.contains(className))
        return;

    m_knownClasses.insert(className);

    if (!m_laidOut && m_uic->customWidgetsInfo()->extends(className, QLatin1String("QToolBox")))
        add(QLatin1String("QLayout")); // spacing property of QToolBox)

    if (className == QLatin1String("Line")) { // ### hmm, deprecate me!
        add(QLatin1String("QFrame"));
        return;
    }

    if (m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3ListView"))  ||
        m_uic->customWidgetsInfo()->extends(className, QLatin1String("Q3Table"))) {
        add(QLatin1String("Q3Header"));
    }
    if (determineHeader)
        insertIncludeForClass(className, header, global);
}

void WriteIncludes::acceptCustomWidget(DomCustomWidget *node)
{
    const QString className = node->elementClass();
    if (className.isEmpty())
        return;

    if (const DomScript *domScript = node->elementScript())
        if (!domScript->text().isEmpty())
            activateScripts();

    if (!node->elementHeader() || node->elementHeader()->text().isEmpty()) {
        add(className, false); // no header specified
    } else {
        // custom header unless it is a built-in qt class
        QString header;
        bool global = false;
        if (!m_classToHeader.contains(className)) {
            global = node->elementHeader()->attributeLocation().toLower() == QLatin1String("global");
            header = node->elementHeader()->text();
        }
        add(className, true, header, global);
    }
}

void WriteIncludes::acceptCustomWidgets(DomCustomWidgets *node)
{
    Q_UNUSED(node);
}

void WriteIncludes::acceptIncludes(DomIncludes *node)
{
    TreeWalker::acceptIncludes(node);
}

void WriteIncludes::acceptInclude(DomInclude *node)
{
    bool global = true;
    if (node->hasAttributeLocation())
        global = node->attributeLocation() == QLatin1String("global");
    insertInclude(node->text(), global);
}

void WriteIncludes::insertInclude(const QString &header, bool global)
{
    if (debugWriteIncludes)
        fprintf(stderr, "%s %s %d\n", Q_FUNC_INFO, qPrintable(header), global);

    OrderedSet &includes = global ?  m_globalIncludes : m_localIncludes;
    if (includes.contains(header))
        return;
    // Insert. Also remember base name for quick check of suspicious custom plugins
    includes.insert(header, false);
    const QString lowerBaseName = QFileInfo(header).completeBaseName ().toLower();
    m_includeBaseNames.insert(lowerBaseName);
}

void WriteIncludes::writeHeaders(const OrderedSet &headers, bool global)
{
    const QChar openingQuote = global ? QLatin1Char('<') : QLatin1Char('"');
    const QChar closingQuote = global ? QLatin1Char('>') : QLatin1Char('"');

    // Check for the old headers 'qslider.h' and replace by 'QtGui/QSlider'
    const OrderedSet::const_iterator cend = headers.constEnd();
    for (OrderedSet::const_iterator sit = headers.constBegin(); sit != cend; ++sit) {
        const StringMap::const_iterator hit = m_oldHeaderToNewHeader.constFind(sit.key());
        const bool mapped =  hit != m_oldHeaderToNewHeader.constEnd();
        const  QString header =  mapped ? hit.value() : sit.key();
        if (!header.trimmed().isEmpty()) {
            m_output << "#include " << openingQuote << header << closingQuote << QLatin1Char('\n');
        }
    }
}

void WriteIncludes::acceptWidgetScripts(const DomScripts &scripts, DomWidget *, const  DomWidgets &)
{
    if (!scripts.empty()) {
        activateScripts();
    }
}

void WriteIncludes::activateScripts()
{
    if (!m_scriptsActivated) {
        add(QLatin1String("QScriptEngine"));
        add(QLatin1String("QDebug"));
        m_scriptsActivated = true;
    }
}
} // namespace CPP

QT_END_NAMESPACE
