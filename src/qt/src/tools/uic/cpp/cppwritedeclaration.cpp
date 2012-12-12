/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "cppwritedeclaration.h"
#include "cppwriteicondeclaration.h"
#include "cppwriteinitialization.h"
#include "cppwriteiconinitialization.h"
#include "cppextractimages.h"
#include "driver.h"
#include "ui4.h"
#include "uic.h"
#include "databaseinfo.h"
#include "customwidgetsinfo.h"

#include <QtCore/QTextStream>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

namespace {
    void openNameSpaces(const QStringList &namespaceList, QTextStream &output) {
        if (namespaceList.empty())
            return;
        const QStringList::const_iterator cend = namespaceList.constEnd();
        for (QStringList::const_iterator it = namespaceList.constBegin(); it != cend; ++it) {
            if (!it->isEmpty()) {
                output << "namespace " << *it << " {\n";
            }
        }
    }

    void closeNameSpaces(const QStringList &namespaceList, QTextStream &output) {
        if (namespaceList.empty())
            return;

        QListIterator<QString> it(namespaceList);
        it.toBack();
        while (it.hasPrevious()) {
            const QString ns = it.previous();
            if (!ns.isEmpty()) {
                output << "} // namespace " << ns << "\n";
            }
        }
    }

    void writeScriptContextClass(const QString &indent, QTextStream &str) {
         str << indent << "class ScriptContext\n"
             << indent << "{\n"
             << indent << "public:\n"
             << indent << "    void run(const QString &script, QWidget *widget, const QWidgetList &childWidgets)\n"
             << indent << "    {\n"
             << indent << "        QScriptValue widgetObject =  scriptEngine.newQObject(widget);\n"
             << indent << "        QScriptValue childWidgetArray = scriptEngine.newArray (childWidgets.size());\n"
             << indent << "        for (int i = 0; i < childWidgets.size(); i++)\n"
             << indent << "               childWidgetArray.setProperty(i, scriptEngine.newQObject(childWidgets[i]));\n"
             << indent << "        QScriptContext *ctx = scriptEngine.pushContext();\n"
             << indent << "        ctx ->activationObject().setProperty(QLatin1String(\"widget\"), widgetObject);\n"
             << indent << "        ctx ->activationObject().setProperty(QLatin1String(\"childWidgets\"), childWidgetArray);\n\n"
             << indent << "        scriptEngine.evaluate(script);\n"
             << indent << "        if (scriptEngine.hasUncaughtException ()) {\n"
             << indent << "            qWarning() << \"An exception occurred at line \" << scriptEngine.uncaughtExceptionLineNumber()\n"
             << indent << "                       << \" of the script for \" << widget->objectName() << \": \" << engineError() << '\\n'\n"
             << indent << "                       << script;\n"
             << indent << "        }\n\n"
             << indent << "        scriptEngine.popContext();\n"
             << indent << "    }\n\n"
             << indent << "private:\n"
             << indent << "    QString engineError()\n"
             << indent << "    {\n"
             << indent << "        QScriptValue error = scriptEngine.evaluate(\"Error\");\n"
             << indent << "        return error.toString();\n"
             << indent << "    }\n\n"
             << indent << "    QScriptEngine scriptEngine;\n"
             << indent << "};\n\n";
    }
}

namespace CPP {

WriteDeclaration::WriteDeclaration(Uic *uic, bool activateScripts)  :
    m_uic(uic),
    m_driver(uic->driver()),
    m_output(uic->output()),
    m_option(uic->option()),
    m_activateScripts(activateScripts)
{
}

void WriteDeclaration::acceptUI(DomUI *node)
{
    QString qualifiedClassName = node->elementClass() + m_option.postfix;
    QString className = qualifiedClassName;

    QString varName = m_driver->findOrInsertWidget(node->elementWidget());
    QString widgetClassName = node->elementWidget()->attributeClass();

    QString exportMacro = node->elementExportMacro();
    if (!exportMacro.isEmpty())
        exportMacro.append(QLatin1Char(' '));

    QStringList namespaceList = qualifiedClassName.split(QLatin1String("::"));
    if (namespaceList.count()) {
        className = namespaceList.last();
        namespaceList.removeLast();
    }

    // This is a bit of the hack but covers the cases Qt in/without namespaces
    // and User defined classes in/without namespaces. The "strange" case
    // is a User using Qt-in-namespace having his own classes not in a namespace.
    // In this case the generated Ui helper classes will also end up in
    // the Qt namespace (which is harmless, but not "pretty")
    const bool needsMacro = namespaceList.count() == 0
        || namespaceList[0] == QLatin1String("qdesigner_internal");

    if (needsMacro)
        m_output << "QT_BEGIN_NAMESPACE\n\n";

    openNameSpaces(namespaceList, m_output);

    if (namespaceList.count())
        m_output << "\n";

    m_output << "class " << exportMacro << m_option.prefix << className << "\n"
           << "{\n"
           << "public:\n";

    const QStringList connections = m_uic->databaseInfo()->connections();
    for (int i=0; i<connections.size(); ++i) {
        const QString connection = connections.at(i);

        if (connection == QLatin1String("(default)"))
            continue;

        m_output << m_option.indent << "QSqlDatabase " << connection << "Connection;\n";
    }

    TreeWalker::acceptWidget(node->elementWidget());
    if (const DomButtonGroups *domButtonGroups = node->elementButtonGroups())
        acceptButtonGroups(domButtonGroups);

    m_output << "\n";

    WriteInitialization(m_uic, m_activateScripts).acceptUI(node);

    if (node->elementImages()) {
        if (m_option.extractImages) {
            ExtractImages(m_uic->option()).acceptUI(node);
        } else {
            m_output << "\n"
                << "protected:\n"
                << m_option.indent << "enum IconID\n"
                << m_option.indent << "{\n";
            WriteIconDeclaration(m_uic).acceptUI(node);

            m_output << m_option.indent << m_option.indent << "unknown_ID\n"
                << m_option.indent << "};\n";

            WriteIconInitialization(m_uic).acceptUI(node);
        }
    }

    if (m_activateScripts) {
        m_output << "\nprivate:\n\n";
        writeScriptContextClass(m_option.indent, m_output);
    }

    m_output << "};\n\n";

    closeNameSpaces(namespaceList, m_output);

    if (namespaceList.count())
        m_output << "\n";

    if (m_option.generateNamespace && !m_option.prefix.isEmpty()) {
        namespaceList.append(QLatin1String("Ui"));

        openNameSpaces(namespaceList, m_output);

        m_output << m_option.indent << "class " << exportMacro << className << ": public " << m_option.prefix << className << " {};\n";

        closeNameSpaces(namespaceList, m_output);

        if (namespaceList.count())
            m_output << "\n";
    }

    if (needsMacro)
        m_output << "QT_END_NAMESPACE\n\n";
}

void WriteDeclaration::acceptWidget(DomWidget *node)
{
    QString className = QLatin1String("QWidget");
    if (node->hasAttributeClass())
        className = node->attributeClass();

    m_output << m_option.indent << m_uic->customWidgetsInfo()->realClassName(className) << " *" << m_driver->findOrInsertWidget(node) << ";\n";

    TreeWalker::acceptWidget(node);
}

void WriteDeclaration::acceptSpacer(DomSpacer *node)
{
     m_output << m_option.indent << "QSpacerItem *" << m_driver->findOrInsertSpacer(node) << ";\n";
     TreeWalker::acceptSpacer(node);
}

void WriteDeclaration::acceptLayout(DomLayout *node)
{
    QString className = QLatin1String("QLayout");
    if (node->hasAttributeClass())
        className = node->attributeClass();

    m_output << m_option.indent << className << " *" << m_driver->findOrInsertLayout(node) << ";\n";

    TreeWalker::acceptLayout(node);
}

void WriteDeclaration::acceptActionGroup(DomActionGroup *node)
{
    m_output << m_option.indent << "QActionGroup *" << m_driver->findOrInsertActionGroup(node) << ";\n";

    TreeWalker::acceptActionGroup(node);
}

void WriteDeclaration::acceptAction(DomAction *node)
{
    m_output << m_option.indent << "QAction *" << m_driver->findOrInsertAction(node) << ";\n";

    TreeWalker::acceptAction(node);
}

void WriteDeclaration::acceptButtonGroup(const DomButtonGroup *buttonGroup)
{
    m_output << m_option.indent << "QButtonGroup *" << m_driver->findOrInsertButtonGroup(buttonGroup) << ";\n";
    TreeWalker::acceptButtonGroup(buttonGroup);
}

} // namespace CPP

QT_END_NAMESPACE
