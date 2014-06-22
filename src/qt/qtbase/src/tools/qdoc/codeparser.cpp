/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

/*
  codeparser.cpp
*/

#include "codeparser.h"
#include "node.h"
#include "tree.h"
#include "config.h"
#include "generator.h"
#include "qdocdatabase.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

#define COMMAND_COMPAT                  Doc::alias(QLatin1String("compat"))
#define COMMAND_DEPRECATED              Doc::alias(QLatin1String("deprecated")) // ### don't document
#define COMMAND_INGROUP                 Doc::alias(QLatin1String("ingroup"))
#define COMMAND_INMODULE                Doc::alias(QLatin1String("inmodule"))  // ### don't document
#define COMMAND_INQMLMODULE             Doc::alias(QLatin1String("inqmlmodule"))
#define COMMAND_INTERNAL                Doc::alias(QLatin1String("internal"))
#define COMMAND_MAINCLASS               Doc::alias(QLatin1String("mainclass"))
#define COMMAND_NONREENTRANT            Doc::alias(QLatin1String("nonreentrant"))
#define COMMAND_OBSOLETE                Doc::alias(QLatin1String("obsolete"))
#define COMMAND_PAGEKEYWORDS            Doc::alias(QLatin1String("pagekeywords"))
#define COMMAND_PRELIMINARY             Doc::alias(QLatin1String("preliminary"))
#define COMMAND_INPUBLICGROUP           Doc::alias(QLatin1String("inpublicgroup"))
#define COMMAND_QTVARIABLE              Doc::alias(QLatin1String("qtvariable"))
#define COMMAND_REENTRANT               Doc::alias(QLatin1String("reentrant"))
#define COMMAND_SINCE                   Doc::alias(QLatin1String("since"))
#define COMMAND_SUBTITLE                Doc::alias(QLatin1String("subtitle"))
#define COMMAND_THREADSAFE              Doc::alias(QLatin1String("threadsafe"))
#define COMMAND_TITLE                   Doc::alias(QLatin1String("title"))
#define COMMAND_WRAPPER                 Doc::alias(QLatin1String("wrapper"))

QString CodeParser::currentSubDir_;
QList<CodeParser *> CodeParser::parsers;
bool CodeParser::showInternal = false;
QMap<QString,QString> CodeParser::nameToTitle;

/*!
  The constructor adds this code parser to the static
  list of code parsers.
 */
CodeParser::CodeParser()
{
    qdb_ = QDocDatabase::qdocDB();
    parsers.prepend(this);
}

/*!
  The destructor removes this code parser from the static
  list of code parsers.
 */
CodeParser::~CodeParser()
{
    parsers.removeAll(this);
}

/*!
  Initialize the code parser base class.
 */
void CodeParser::initializeParser(const Config& config)
{
    showInternal = config.getBool(CONFIG_SHOWINTERNAL);
}

/*!
  Terminating a code parser is trivial.
 */
void CodeParser::terminateParser()
{
    // nothing.
}

QStringList CodeParser::headerFileNameFilter()
{
    return sourceFileNameFilter();
}

void CodeParser::parseHeaderFile(const Location& location, const QString& filePath)
{
    parseSourceFile(location, filePath);
}

void CodeParser::doneParsingHeaderFiles()
{
    doneParsingSourceFiles();
}

/*!
  All the code parsers in the static list are initialized here,
  after the qdoc configuration variables have been set.
 */
void CodeParser::initialize(const Config& config)
{
    QList<CodeParser *>::ConstIterator p = parsers.constBegin();
    while (p != parsers.constEnd()) {
        (*p)->initializeParser(config);
        ++p;
    }
}

/*!
  All the code parsers in the static list are terminated here.
 */
void CodeParser::terminate()
{
    QList<CodeParser *>::ConstIterator p = parsers.constBegin();
    while (p != parsers.constEnd()) {
        (*p)->terminateParser();
        ++p;
    }
}

CodeParser *CodeParser::parserForLanguage(const QString& language)
{
    QList<CodeParser *>::ConstIterator p = parsers.constBegin();
    while (p != parsers.constEnd()) {
        if ((*p)->language() == language)
            return *p;
        ++p;
    }
    return 0;
}

CodeParser *CodeParser::parserForHeaderFile(const QString &filePath)
{
    QString fileName = QFileInfo(filePath).fileName();

    QList<CodeParser *>::ConstIterator p = parsers.constBegin();
    while (p != parsers.constEnd()) {

        QStringList headerPatterns = (*p)->headerFileNameFilter();
        foreach (const QString &pattern, headerPatterns) {
            QRegExp re(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
            if (re.exactMatch(fileName))
                return *p;
        }
        ++p;
    }
    return 0;
}

CodeParser *CodeParser::parserForSourceFile(const QString &filePath)
{
    QString fileName = QFileInfo(filePath).fileName();

    QList<CodeParser *>::ConstIterator p = parsers.constBegin();
    while (p != parsers.constEnd()) {

        QStringList sourcePatterns = (*p)->sourceFileNameFilter();
        foreach (const QString &pattern, sourcePatterns) {
            QRegExp re(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
            if (re.exactMatch(fileName))
                return *p;
        }
        ++p;
    }
    return 0;
}

static QSet<QString> commonMetaCommands_;
/*!
  Returns the set of strings representing the common metacommands.
 */
const QSet<QString>& CodeParser::commonMetaCommands()
{
    if (commonMetaCommands_.isEmpty()) {
        commonMetaCommands_ << COMMAND_COMPAT
                            << COMMAND_DEPRECATED
                            << COMMAND_INGROUP
                            << COMMAND_INMODULE
                            << COMMAND_INQMLMODULE
                            << COMMAND_INTERNAL
                            << COMMAND_MAINCLASS
                            << COMMAND_NONREENTRANT
                            << COMMAND_OBSOLETE
                            << COMMAND_PAGEKEYWORDS
                            << COMMAND_PRELIMINARY
                            << COMMAND_INPUBLICGROUP
                            << COMMAND_QTVARIABLE
                            << COMMAND_REENTRANT
                            << COMMAND_SINCE
                            << COMMAND_SUBTITLE
                            << COMMAND_THREADSAFE
                            << COMMAND_TITLE
                            << COMMAND_WRAPPER;
    }
    return commonMetaCommands_;
}

/*!
  The topic command has been processed. Now process the other
  metacommands that were found. These are not the text markup
  commands.
 */
void CodeParser::processCommonMetaCommand(const Location& location,
                                          const QString& command,
                                          const ArgLocPair& arg,
                                          Node* node)
{
    if (command == COMMAND_COMPAT) {
        location.warning(tr("\\compat command used, but Qt3 compatibility is no longer supported"));
        node->setStatus(Node::Compat);
    }
    else if (command == COMMAND_DEPRECATED) {
        node->setStatus(Node::Obsolete);
    }
    else if ((command == COMMAND_INGROUP) || (command == COMMAND_INPUBLICGROUP)) {
        // Note: \ingroup and \inpublicgroup are now the same.
        // Not that they were ever different.
        qdb_->addToGroup(arg.first, node);
    }
    else if (command == COMMAND_INMODULE) {
        qdb_->addToModule(arg.first,node);
    }
    else if (command == COMMAND_INQMLMODULE) {
        qdb_->addToQmlModule(arg.first,node);
    }
    else if (command == COMMAND_MAINCLASS) {
        node->setStatus(Node::Main);
    }
    else if (command == COMMAND_OBSOLETE) {
        if (node->status() != Node::Compat)
            node->setStatus(Node::Obsolete);
    }
    else if (command == COMMAND_NONREENTRANT) {
        node->setThreadSafeness(Node::NonReentrant);
    }
    else if (command == COMMAND_PRELIMINARY) {
        node->setStatus(Node::Preliminary);
    }
    else if (command == COMMAND_INTERNAL) {
        if (!showInternal) {
            node->setAccess(Node::Private);
            node->setStatus(Node::Internal);
            if (node->type() == Node::QmlPropertyGroup) {
                const QmlPropertyGroupNode* qpgn = static_cast<const QmlPropertyGroupNode*>(node);
                NodeList::ConstIterator p = qpgn->childNodes().constBegin();
                while (p != qpgn->childNodes().constEnd()) {
                    if ((*p)->type() == Node::QmlProperty) {
                        (*p)->setAccess(Node::Private);
                        (*p)->setStatus(Node::Internal);
                    }
                    ++p;
                }
            }
        }
    }
    else if (command == COMMAND_REENTRANT) {
        node->setThreadSafeness(Node::Reentrant);
    }
    else if (command == COMMAND_SINCE) {
        node->setSince(arg.first);
    }
    else if (command == COMMAND_WRAPPER) {
        node->setWrapper();
    }
    else if (command == COMMAND_PAGEKEYWORDS) {
        node->addPageKeywords(arg.first);
    }
    else if (command == COMMAND_SUBTITLE) {
        if (node->type() == Node::Document) {
            DocNode *dn = static_cast<DocNode *>(node);
            dn->setSubTitle(arg.first);
        }
        else
            location.warning(tr("Ignored '\\%1'").arg(COMMAND_SUBTITLE));
    }
    else if (command == COMMAND_THREADSAFE) {
        node->setThreadSafeness(Node::ThreadSafe);
    }
    else if (command == COMMAND_TITLE) {
        if (node->type() == Node::Document) {
            DocNode *dn = static_cast<DocNode *>(node);
            dn->setTitle(arg.first);
            if (dn->subType() == Node::Example) {
                ExampleNode::exampleNodeMap.insert(dn->title(),static_cast<ExampleNode*>(dn));
            }
            nameToTitle.insert(dn->name(),arg.first);
        }
        else
            location.warning(tr("Ignored '\\%1'").arg(COMMAND_TITLE));
    }
    else if (command == COMMAND_QTVARIABLE) {
        if (node->subType() == Node::Module) {
            DocNode *dn = static_cast<DocNode *>(node);
            dn->setQtVariable(arg.first);
        }
        else
            location.warning(tr("Command '\\%1' found outside of '\\module'. It can only be used within a module page.")
                             .arg(COMMAND_QTVARIABLE));
    }
}

/*!
  Find the page title given the page \a name and return it.
 */
const QString CodeParser::titleFromName(const QString& name)
{
    const QString t = nameToTitle.value(name);
    return t;
}

/*!
  \internal
 */
void CodeParser::extractPageLinkAndDesc(const QString& arg,
                                        QString* link,
                                        QString* desc)
{
    QRegExp bracedRegExp(QLatin1String("\\{([^{}]*)\\}(?:\\{([^{}]*)\\})?"));

    if (bracedRegExp.exactMatch(arg)) {
        *link = bracedRegExp.cap(1);
        *desc = bracedRegExp.cap(2);
        if (desc->isEmpty())
            *desc = *link;
    }
    else {
        int spaceAt = arg.indexOf(QLatin1Char(' '));
        if (arg.contains(QLatin1String(".html")) && spaceAt != -1) {
            *link = arg.left(spaceAt).trimmed();
            *desc = arg.mid(spaceAt).trimmed();
        }
        else {
            *link = arg;
            *desc = arg;
        }
    }
}

/*!
  \internal
 */
void CodeParser::setLink(Node* node, Node::LinkType linkType, const QString& arg)
{
    QString link;
    QString desc;
    extractPageLinkAndDesc(arg, &link, &desc);
    node->setLink(linkType, link, desc);
}

/*!
  Returns \c true if the file being parsed is a .h file.
 */
bool CodeParser::isParsingH() const
{
    return currentFile_.endsWith(".h");
}

/*!
  Returns \c true if the file being parsed is a .cpp file.
 */
bool CodeParser::isParsingCpp() const
{
    return currentFile_.endsWith(".cpp");
}

/*!
  Returns \c true if the file being parsed is a .qdoc file.
 */
bool CodeParser::isParsingQdoc() const
{
    return currentFile_.endsWith(".qdoc");
}

/*!
  For each node that will produce a documentation page, this function
  ensures that the node belongs to a module. Normally, the qdoc comment
  for an entity that will produce a documentation page will contain an
  \inmodule command to tell qdoc which module the entity belongs to.

  But now that we normally run qdoc on each module in two passes. The
  first produces an index file; the second pass generates the docs
  after reading all the index files it needs.

  This means that all the pages generated during each pass 2 run of
  qdoc almost certainly belong to a single module, and the name of
  that module is, as a rule, used as the project name in the qdocconf
  file used when running qdoc on the module.

  So this function first asks if the node \a n has a non-empty module
  name. If it it does not have a non-empty module name, it sets the
  module name to be the project name.

  In some cases it prints a qdoc warning that it has done this. Namely,
  for C++ classes and namespaces.
 */
void CodeParser::checkModuleInclusion(Node* n)
{
    if (n->moduleName().isEmpty()) {
        switch (n->type()) {
        case Node::Class:
            if (n->access() != Node::Private && !n->doc().isEmpty()) {
                n->setModuleName(Generator::defaultModuleName());
                n->doc().location().warning(tr("Class %1 has no \\inmodule command; "
                                               "using project name by default: %2")
                                            .arg(n->name()).arg(Generator::defaultModuleName()));
            }
            break;
        case Node::Namespace:
            if (n->access() != Node::Private && !n->name().isEmpty() && !n->doc().isEmpty()) {
                n->setModuleName(Generator::defaultModuleName());
                n->doc().location().warning(tr("Namespace %1 has no \\inmodule command; "
                                               "using project name by default: %2")
                                            .arg(n->name()).arg(Generator::defaultModuleName()));
            }
            break;
        case Node::Document:
            if (n->access() != Node::Private && !n->doc().isEmpty()) {
                if (n->subType() == Node::HeaderFile) {
                    n->setModuleName(Generator::defaultModuleName());
#if 0
                    n->doc().location().warning(tr("Header file with title \"%1\" has no \\inmodule command; "
                                                   "using project name by default: %2")
                                                .arg(n->title()).arg(Generator::defaultModuleName()));
#endif
                }
                else if (n->subType() == Node::Page) {
                    n->setModuleName(Generator::defaultModuleName());
#if 0
                    n->doc().location().warning(tr("Page with title \"%1\" has no \\inmodule command; "
                                                   "using project name by default: %2")
                                                .arg(n->title()).arg(Generator::defaultModuleName()));
#endif
                }
                else if (n->subType() == Node::Example) {
                    n->setModuleName(Generator::defaultModuleName());
#if 0
                    n->doc().location().warning(tr("Example with title \"%1\" has no \\inmodule command; "
                                                   "using project name by default: %2")
                                                .arg(n->title()).arg(Generator::defaultModuleName()));
#endif
                }
            }
            break;
        default:
            break;
        }
    }
}

QT_END_NAMESPACE
