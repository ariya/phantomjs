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
  cppcodeparser.cpp
*/

#include <qfile.h>
#include <stdio.h>
#include <errno.h>
#include "codechunk.h"
#include "config.h"
#include "cppcodeparser.h"
#include "tokenizer.h"
#include "qdocdatabase.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/* qmake ignore Q_OBJECT */

static bool inMacroCommand_ = false;
QStringList CppCodeParser::exampleFiles;
QStringList CppCodeParser::exampleDirs;

/*!
  The constructor initializes some regular expressions
  and calls reset().
 */
CppCodeParser::CppCodeParser()
    : varComment("/\\*\\s*([a-zA-Z_0-9]+)\\s*\\*/"), sep("(?:<[^>]+>)?::")
{
    reset();
}

/*!
  The destructor is trivial.
 */
CppCodeParser::~CppCodeParser()
{
    // nothing.
}

/*!
  The constructor initializes a map of special node types
  for identifying important nodes. And it initializes
  some filters for identifying certain kinds of files.
 */
void CppCodeParser::initializeParser(const Config &config)
{
    CodeParser::initializeParser(config);

    /*
      All these can appear in a C++ namespace. Don't add
      anything that can't be in a C++ namespace.
     */
    nodeTypeMap.insert(COMMAND_NAMESPACE, Node::Namespace);
    nodeTypeMap.insert(COMMAND_CLASS, Node::Class);
    nodeTypeMap.insert(COMMAND_ENUM, Node::Enum);
    nodeTypeMap.insert(COMMAND_TYPEDEF, Node::Typedef);
    nodeTypeMap.insert(COMMAND_PROPERTY, Node::Property);
    nodeTypeMap.insert(COMMAND_VARIABLE, Node::Variable);

    exampleFiles = config.getCleanPathList(CONFIG_EXAMPLES);
    exampleDirs = config.getCleanPathList(CONFIG_EXAMPLEDIRS);
    QStringList exampleFilePatterns = config.getStringList(
                CONFIG_EXAMPLES + Config::dot + CONFIG_FILEEXTENSIONS);

    if (!exampleFilePatterns.isEmpty())
        exampleNameFilter = exampleFilePatterns.join(' ');
    else
        exampleNameFilter = "*.cpp *.h *.js *.xq *.svg *.xml *.dita *.ui";

    QStringList exampleImagePatterns = config.getStringList(
                CONFIG_EXAMPLES + Config::dot + CONFIG_IMAGEEXTENSIONS);

    if (!exampleImagePatterns.isEmpty())
        exampleImageFilter = exampleImagePatterns.join(' ');
    else
        exampleImageFilter = "*.png";
}

/*!
  Clear the map of common node types and call
  the same function in the base class.
 */
void CppCodeParser::terminateParser()
{
    nodeTypeMap.clear();
    CodeParser::terminateParser();
}

/*!
  Returns "Cpp".
 */
QString CppCodeParser::language()
{
    return "Cpp";
}

/*!
  Returns a list of extensions for header files.
 */
QStringList CppCodeParser::headerFileNameFilter()
{
    return QStringList() << "*.ch" << "*.h" << "*.h++" << "*.hh" << "*.hpp" << "*.hxx";
}

/*!
  Returns a list of extensions for source files, i.e. not
  header files.
 */
QStringList CppCodeParser::sourceFileNameFilter()
{
    return QStringList() << "*.c++" << "*.cc" << "*.cpp" << "*.cxx" << "*.mm";
}

/*!
  Parse the C++ header file identified by \a filePath and add
  the parsed contents to the database. The \a location is used
  for reporting errors.
 */
void CppCodeParser::parseHeaderFile(const Location& location, const QString& filePath)
{
    QFile in(filePath);
    currentFile_ = filePath;
    if (!in.open(QIODevice::ReadOnly)) {
        location.error(tr("Cannot open C++ header file '%1'").arg(filePath));
        currentFile_.clear();
        return;
    }

    reset();
    Location fileLocation(filePath);
    Tokenizer fileTokenizer(fileLocation, in);
    tokenizer = &fileTokenizer;
    readToken();
    matchDeclList(qdb_->treeRoot());
    if (!fileTokenizer.version().isEmpty())
        qdb_->setVersion(fileTokenizer.version());
    in.close();

    if (fileLocation.fileName() == "qiterator.h")
        parseQiteratorDotH(location, filePath);
    currentFile_.clear();
}

/*!
  Get ready to parse the C++ cpp file identified by \a filePath
  and add its parsed contents to the database. \a location is
  used for reporting errors.

  Call matchDocsAndStuff() to do all the parsing and tree building.
 */
void CppCodeParser::parseSourceFile(const Location& location, const QString& filePath)
{
    QFile in(filePath);
    currentFile_ = filePath;
    if (!in.open(QIODevice::ReadOnly)) {
        location.error(tr("Cannot open C++ source file '%1' (%2)").arg(filePath).arg(strerror(errno)));
        currentFile_.clear();
        return;
    }

    reset();
    Location fileLocation(filePath);
    Tokenizer fileTokenizer(fileLocation, in);
    tokenizer = &fileTokenizer;
    readToken();

    /*
      The set of open namespaces is cleared before parsing
      each source file. The word "source" here means cpp file.
     */
    qdb_->clearOpenNamespaces();

    matchDocsAndStuff();
    in.close();
    currentFile_.clear();
}

/*!
  This is called after all the header files have been parsed.
  I think the most important thing it does is resolve class
  inheritance links in the tree. But it also initializes a
  bunch of stuff.
 */
void CppCodeParser::doneParsingHeaderFiles()
{
    qdb_->resolveInheritance();

    QMapIterator<QString, QString> i(sequentialIteratorClasses);
    while (i.hasNext()) {
        i.next();
        instantiateIteratorMacro(i.key(), i.value(), sequentialIteratorDefinition);
    }
    i = mutableSequentialIteratorClasses;
    while (i.hasNext()) {
        i.next();
        instantiateIteratorMacro(i.key(), i.value(), mutableSequentialIteratorDefinition);
    }
    i = associativeIteratorClasses;
    while (i.hasNext()) {
        i.next();
        instantiateIteratorMacro(i.key(), i.value(), associativeIteratorDefinition);
    }
    i = mutableAssociativeIteratorClasses;
    while (i.hasNext()) {
        i.next();
        instantiateIteratorMacro(i.key(), i.value(), mutableAssociativeIteratorDefinition);
    }
    sequentialIteratorDefinition.clear();
    mutableSequentialIteratorDefinition.clear();
    associativeIteratorDefinition.clear();
    mutableAssociativeIteratorDefinition.clear();
    sequentialIteratorClasses.clear();
    mutableSequentialIteratorClasses.clear();
    associativeIteratorClasses.clear();
    mutableAssociativeIteratorClasses.clear();
}

/*!
  This is called after all the source files (i.e., not the
  header files) have been parsed. It traverses the tree to
  resolve property links, normalize overload signatures, and
  do other housekeeping of the database.
 */
void CppCodeParser::doneParsingSourceFiles()
{
    qdb_->treeRoot()->clearCurrentChildPointers();
    qdb_->treeRoot()->normalizeOverloads();
    qdb_->fixInheritance();
    qdb_->resolveProperties();
    qdb_->treeRoot()->makeUndocumentedChildrenInternal();
}

static QSet<QString> topicCommands_;
/*!
  Returns the set of strings reopresenting the topic commands.
 */
const QSet<QString>& CppCodeParser::topicCommands()
{
    if (topicCommands_.isEmpty()) {
        topicCommands_ << COMMAND_CLASS
                       << COMMAND_DITAMAP
                       << COMMAND_ENUM
                       << COMMAND_EXAMPLE
                       << COMMAND_EXTERNALPAGE
                       << COMMAND_FILE
                       << COMMAND_FN
                       << COMMAND_GROUP
                       << COMMAND_HEADERFILE
                       << COMMAND_MACRO
                       << COMMAND_MODULE
                       << COMMAND_NAMESPACE
                       << COMMAND_PAGE
                       << COMMAND_PROPERTY
                       << COMMAND_TYPEDEF
                       << COMMAND_VARIABLE
                       << COMMAND_QMLCLASS
                       << COMMAND_QMLTYPE
                       << COMMAND_QMLPROPERTY
                       << COMMAND_QMLPROPERTYGROUP
                       << COMMAND_QMLATTACHEDPROPERTY
                       << COMMAND_QMLSIGNAL
                       << COMMAND_QMLATTACHEDSIGNAL
                       << COMMAND_QMLMETHOD
                       << COMMAND_QMLATTACHEDMETHOD
                       << COMMAND_QMLBASICTYPE
                       << COMMAND_QMLMODULE;
    }
    return topicCommands_;
}

/*!
  Process the topic \a command found in the \a doc with argument \a arg.
 */
Node* CppCodeParser::processTopicCommand(const Doc& doc,
                                         const QString& command,
                                         const ArgLocPair& arg)
{
    ExtraFuncData extra;
    if (command == COMMAND_FN) {
        QStringList parentPath;
        FunctionNode *func = 0;
        FunctionNode *clone = 0;

        if (!makeFunctionNode(arg.first, &parentPath, &clone, extra) &&
            !makeFunctionNode("void " + arg.first, &parentPath, &clone, extra)) {
            doc.startLocation().warning(tr("Invalid syntax in '\\%1'").arg(COMMAND_FN));
        }
        else {
            func = qdb_->findNodeInOpenNamespace(parentPath, clone);
            /*
              Search the root namespace if no match was found.
            */
            if (func == 0) {
                func = qdb_->findFunctionNode(parentPath, clone);
            }

            if (func == 0) {
                if (parentPath.isEmpty() && !lastPath_.isEmpty()) {
                    func = qdb_->findFunctionNode(lastPath_, clone);
                }
                if (func == 0) {
                    doc.location().warning(tr("Cannot find '%1' in '\\%2' %3")
                                           .arg(clone->name() + "(...)")
                                           .arg(COMMAND_FN)
                                           .arg(arg.first),
                                           tr("I cannot find any function of that name with the "
                                              "specified signature. Make sure that the signature "
                                              "is identical to the declaration, including 'const' "
                                              "qualifiers."));
                }
                else {
                    doc.location().warning(tr("Missing '%1::' for '%2' in '\\%3'")
                                           .arg(lastPath_.join("::"))
                                           .arg(clone->name() + "()")
                                           .arg(COMMAND_FN));
                }
            }
            else {
                lastPath_ = parentPath;
            }
            if (func) {
                func->borrowParameterNames(clone);
                func->setParentPath(clone->parentPath());
            }
            delete clone;
        }
        return func;
    }
    else if (command == COMMAND_MACRO) {
        QStringList parentPath;
        FunctionNode *func = 0;

        extra.root = qdb_->treeRoot();
        extra.isMacro = true;
        if (makeFunctionNode(arg.first, &parentPath, &func, extra)) {
            if (!parentPath.isEmpty()) {
                doc.startLocation().warning(tr("Invalid syntax in '\\%1'").arg(COMMAND_MACRO));
                delete func;
                func = 0;
            }
            else {
                func->setMetaness(FunctionNode::MacroWithParams);
                QList<Parameter> params = func->parameters();
                for (int i = 0; i < params.size(); ++i) {
                    Parameter &param = params[i];
                    if (param.name().isEmpty() && !param.leftType().isEmpty()
                            && param.leftType() != "...")
                        param = Parameter("", "", param.leftType());
                }
                func->setParameters(params);
            }
            return func;
        }
        else if (QRegExp("[A-Za-z_][A-Za-z0-9_]+").exactMatch(arg.first)) {
            func = new FunctionNode(qdb_->treeRoot(), arg.first);
            func->setAccess(Node::Public);
            func->setLocation(doc.startLocation());
            func->setMetaness(FunctionNode::MacroWithoutParams);
        }
        else {
            doc.location().warning(tr("Invalid syntax in '\\%1'").arg(COMMAND_MACRO));

        }
        return func;
    }
    else if (nodeTypeMap.contains(command)) {
        /*
          We should only get in here if the command refers to
          something that can appear in a C++ namespace,
          i.e. a class, another namespace, an enum, a typedef,
          a property or a variable. I think these are handled
          this way to allow the writer to refer to the entity
          without including the namespace qualifier.
         */
        Node::Type type =  nodeTypeMap[command];
        Node::SubType subtype = Node::NoSubType;
        if (type == Node::Document)
            subtype = Node::QmlClass;

        QStringList paths = arg.first.split(QLatin1Char(' '));
        QStringList path = paths[0].split("::");
        Node *node = 0;

        /*
          If the command refers to something that can be in a
          C++ namespace, search for it first in all the known
          C++ namespaces.
         */
        node = qdb_->findNodeInOpenNamespace(path, type, subtype);

        /*
          If the node was not found in a C++ namespace, search
          for it in the root namespace.
         */
        if (node == 0) {
            node = qdb_->findNodeByNameAndType(path, type, subtype);
        }

        if (node == 0) {
            doc.location().warning(tr("Cannot find '%1' specified with '\\%2' in any header file")
                                   .arg(arg.first).arg(command));
            lastPath_ = path;

        }
        else if (node->isInnerNode()) {
            /*
              This treets a class as a namespace.
             */
            if (path.size() > 1) {
                path.pop_back();
                QString ns = path.join("::");
                qdb_->insertOpenNamespace(ns);
            }
        }
        return node;
    }
    else if (command == COMMAND_EXAMPLE) {
        if (Config::generateExamples) {
            ExampleNode* en = new ExampleNode(qdb_->treeRoot(), arg.first);
            en->setLocation(doc.startLocation());
            createExampleFileNodes(en);
            return en;
        }
    }
    else if (command == COMMAND_EXTERNALPAGE) {
        DocNode* dn = new DocNode(qdb_->treeRoot(), arg.first, Node::ExternalPage, Node::ArticlePage);
        dn->setLocation(doc.startLocation());
        return dn;
    }
    else if (command == COMMAND_FILE) {
        DocNode* dn = new DocNode(qdb_->treeRoot(), arg.first, Node::File, Node::NoPageType);
        dn->setLocation(doc.startLocation());
        return dn;
    }
    else if (command == COMMAND_GROUP) {
        DocNode* dn = qdb_->addGroup(arg.first);
        dn->setLocation(doc.startLocation());
        return dn;
    }
    else if (command == COMMAND_HEADERFILE) {
        DocNode* dn = new DocNode(qdb_->treeRoot(), arg.first, Node::HeaderFile, Node::ApiPage);
        dn->setLocation(doc.startLocation());
        return dn;
    }
    else if (command == COMMAND_MODULE) {
        DocNode* dn = qdb_->addModule(arg.first);
        dn->setLocation(doc.startLocation());
        return dn;
    }
    else if (command == COMMAND_QMLMODULE) {
        DocNode* dn = qdb_->addQmlModule(arg.first);
        dn->setLocation(doc.startLocation());
        return dn;
    }
    else if (command == COMMAND_PAGE) {
        Node::PageType ptype = Node::ArticlePage;
        QStringList args = arg.first.split(QLatin1Char(' '));
        if (args.size() > 1) {
            QString t = args[1].toLower();
            if (t == "howto")
                ptype = Node::HowToPage;
            else if (t == "api")
                ptype = Node::ApiPage;
            else if (t == "example")
                ptype = Node::ExamplePage;
            else if (t == "overview")
                ptype = Node::OverviewPage;
            else if (t == "tutorial")
                ptype = Node::TutorialPage;
            else if (t == "faq")
                ptype = Node::FAQPage;
            else if (t == "ditamap")
                ptype = Node::DitaMapPage;
        }

        /*
          Search for a node with the same name. If there is one,
          then there is a collision, so create a collision node
          and make the existing node a child of the collision
          node, and then create the new Page node and make
          it a child of the collision node as well. Return the
          collision node.

          If there is no collision, just create a new Page
          node and return that one.
        */
        NameCollisionNode* ncn = qdb_->checkForCollision(args[0]);
        DocNode* dn = 0;
        if (ptype == Node::DitaMapPage)
            dn = new DitaMapNode(qdb_->treeRoot(), args[0]);
        else
            dn = new DocNode(qdb_->treeRoot(), args[0], Node::Page, ptype);
        dn->setLocation(doc.startLocation());
        if (ncn) {
            ncn->addCollision(dn);
        }
        return dn;
    }
    else if (command == COMMAND_DITAMAP) {
        DocNode* dn = new DitaMapNode(qdb_->treeRoot(), arg.first);
        dn->setLocation(doc.startLocation());
        return dn;
    }
    else if ((command == COMMAND_QMLCLASS) || (command == COMMAND_QMLTYPE)) {
        if (command == COMMAND_QMLCLASS)
            doc.startLocation().warning(tr("\\qmlclass is deprecated; use \\qmltype instead"));
        ClassNode* classNode = 0;
        QStringList names = arg.first.split(QLatin1Char(' '));
        if (names.size() > 1) {
            if (names[1] != "0")
                doc.startLocation().warning(tr("\\qmltype no longer has a 2nd argument; "
                                               "use '\\instantiates <class>' in \\qmltype "
                                               "comments instead"));
            else
                doc.startLocation().warning(tr("The 0 arg is no longer used for indicating "
                                               "that the QML type does not instantiate a "
                                               "C++ class"));
            /*
              If the second argument of the \\qmlclass command
              is 0 we should ignore the C++ class. The second
              argument should only be 0 when you are documenting
              QML in a .qdoc file.
             */
            if (names[1] != "0")
                classNode = qdb_->findClassNode(names[1].split("::"));
        }

        /*
          Search for a node with the same name. If there is one,
          then there is a collision, so create a collision node
          and make the existing node a child of the collision
          node, and then create the new QML class node and make
          it a child of the collision node as well. Return the
          collision node.

          If there is no collision, just create a new QML class
          node and return that one.
         */
        NameCollisionNode* ncn = qdb_->checkForCollision(names[0]);
        QmlClassNode* qcn = new QmlClassNode(qdb_->treeRoot(), names[0]);
        qcn->setClassNode(classNode);
        qcn->setLocation(doc.startLocation());
#if 0
        // to be removed if \qmltype and \instantiates work ok
        if (isParsingCpp() || isParsingQdoc()) {
            qcn->requireCppClass();
            if (names.size() < 2) {
                QString msg = "C++ class name not specified for class documented as "
                    "QML type: '\\qmlclass " + arg.first + " <class name>'";
                doc.startLocation().warning(tr(msg.toLatin1().data()));
            }
            else if (!classNode) {
                QString msg = "C++ class not found in any .h file for class documented "
                    "as QML type: '\\qmlclass " + arg.first + "'";
                doc.startLocation().warning(tr(msg.toLatin1().data()));
            }
        }
#endif
        if (ncn)
            ncn->addCollision(qcn);
        return qcn;
    }
    else if (command == COMMAND_QMLBASICTYPE) {
        QmlBasicTypeNode* n = new QmlBasicTypeNode(qdb_->treeRoot(), arg.first);
        n->setLocation(doc.startLocation());
        return n;
    }
    else if ((command == COMMAND_QMLSIGNAL) ||
             (command == COMMAND_QMLMETHOD) ||
             (command == COMMAND_QMLATTACHEDSIGNAL) ||
             (command == COMMAND_QMLATTACHEDMETHOD)) {
        QString module;
        QString qmlType;
        QString type;
        if (splitQmlMethodArg(arg.first,type,module,qmlType)) {
            QmlClassNode* qmlClass = qdb_->findQmlType(module,qmlType);
            if (qmlClass) {
                bool attached = false;
                Node::Type nodeType = Node::QmlMethod;
                if (command == COMMAND_QMLSIGNAL)
                    nodeType = Node::QmlSignal;
                else if (command == COMMAND_QMLATTACHEDSIGNAL) {
                    nodeType = Node::QmlSignal;
                    attached = true;
                }
                else if (command == COMMAND_QMLMETHOD) {
                    // do nothing
                }
                else if (command == COMMAND_QMLATTACHEDMETHOD)
                    attached = true;
                else
                    return 0; // never get here.
                FunctionNode* fn = makeFunctionNode(doc,
                                                    arg.first,
                                                    qmlClass,
                                                    nodeType,
                                                    attached,
                                                    command);
                if (fn)
                    fn->setLocation(doc.startLocation());
                return fn;
            }
        }
    }
    return 0;
}

/*!
  A QML property group argument has the form...

  <QML-module>::<QML-type>::<name>

  This function splits the argument into those parts.
  A <QML-module> is the QML equivalent of a C++ namespace.
  So this function splits \a arg on "::" and stores the
  parts in \a module, \a qmlType, and \a name, and returns
  true. If any part is not found, a qdoc warning is emitted
  and false is returned.
 */
bool CppCodeParser::splitQmlPropertyGroupArg(const QString& arg,
                                             QString& module,
                                             QString& qmlType,
                                             QString& name)
{
    QStringList colonSplit = arg.split("::");
    if (colonSplit.size() == 3) {
        module = colonSplit[0];
        qmlType = colonSplit[1];
        name = colonSplit[2];
        return true;
    }
    QString msg = "Unrecognizable QML module/component qualifier for " + arg;
    location().warning(tr(msg.toLatin1().data()));
    return false;
}

/*!
  A QML property argument has the form...

  <type> <QML-type>::<name>
  <type> <QML-module>::<QML-type>::<name>

  This function splits the argument into one of those
  two forms. The three part form is the old form, which
  was used before the creation of Qt Quick 2 and Qt
  Components. A <QML-module> is the QML equivalent of a
  C++ namespace. So this function splits \a arg on "::"
  and stores the parts in \a type, \a module, \a qmlType,
  and \a name, and returns \c true. If any part other than
  \a module is not found, a qdoc warning is emitted and
  false is returned.

  \note The two QML types \e{Component} and \e{QtObject}
  never have a module qualifier.
 */
bool CppCodeParser::splitQmlPropertyArg(const QString& arg,
                                        QString& type,
                                        QString& module,
                                        QString& qmlType,
                                        QString& name)
{
    QStringList blankSplit = arg.split(QLatin1Char(' '));
    if (blankSplit.size() > 1) {
        type = blankSplit[0];
        QStringList colonSplit(blankSplit[1].split("::"));
        if (colonSplit.size() == 3) {
            module = colonSplit[0];
            qmlType = colonSplit[1];
            name = colonSplit[2];
            return true;
        }
        if (colonSplit.size() == 2) {
            module.clear();
            qmlType = colonSplit[0];
            name = colonSplit[1];
            return true;
        }
        QString msg = "Unrecognizable QML module/component qualifier for " + arg;
        location().warning(tr(msg.toLatin1().data()));
    }
    else {
        QString msg = "Missing property type for " + arg;
        location().warning(tr(msg.toLatin1().data()));
    }
    return false;
}

/*!
  A QML signal or method argument has the form...

  <type> <QML-type>::<name>(<param>, <param>, ...)
  <type> <QML-module>::<QML-type>::<name>(<param>, <param>, ...)

  This function splits the argument into one of those two
  forms, sets \a module, \a qmlType, and \a name, and returns
  true. If the argument doesn't match either form, an error
  message is emitted and false is returned.

  \note The two QML types \e{Component} and \e{QtObject} never
  have a module qualifier.
 */
bool CppCodeParser::splitQmlMethodArg(const QString& arg,
                                      QString& type,
                                      QString& module,
                                      QString& qmlType)
{
    QStringList colonSplit(arg.split("::"));
    if (colonSplit.size() > 1) {
        QStringList blankSplit = colonSplit[0].split(QLatin1Char(' '));
        if (blankSplit.size() > 1) {
            type = blankSplit[0];
            if (colonSplit.size() > 2) {
                module = blankSplit[1];
                qmlType = colonSplit[1];
            }
            else {
                module.clear();
                qmlType = blankSplit[1];
            }
        }
        else {
            type.clear();
            if (colonSplit.size() > 2) {
                module = colonSplit[0];
                qmlType = colonSplit[1];
            }
            else {
                module.clear();
                qmlType = colonSplit[0];
            }
        }
        return true;
    }
    QString msg = "Unrecognizable QML module/component qualifier for " + arg;
    location().warning(tr(msg.toLatin1().data()));
    return false;
}

/*!
  Process the topic \a command group found in the \a doc with arguments \a args.

  Currently, this function is called only for \e{qmlproperty}
  and \e{qmlattachedproperty}.
 */
void CppCodeParser::processQmlProperties(const Doc& doc, NodeList& nodes, DocList& docs)
{
    QString arg;
    QString type;
    QString topic;
    QString module;
    QString qmlType;
    QString property;
    QmlPropertyNode* qpn = 0;
    QmlClassNode* qmlClass = 0;
    QmlPropertyGroupNode* qpgn = 0;

    Topic qmlPropertyGroupTopic;
    const TopicList& topics = doc.topicsUsed();
    for (int i=0; i<topics.size(); ++i) {
        if (topics.at(i).topic == COMMAND_QMLPROPERTYGROUP) {
            qmlPropertyGroupTopic = topics.at(i);
            break;
        }
    }
    if (qmlPropertyGroupTopic.isEmpty() && topics.size() > 1) {
        qmlPropertyGroupTopic = topics.at(0);
        qmlPropertyGroupTopic.topic = COMMAND_QMLPROPERTYGROUP;
        arg = qmlPropertyGroupTopic.args;
        if (splitQmlPropertyArg(arg, type, module, qmlType, property)) {
            int i = property.indexOf('.');
            if (i != -1) {
                property = property.left(i);
                qmlPropertyGroupTopic.args = module + "::" + qmlType + "::" + property;
                doc.location().warning(tr("No QML property group command found; using \\%1 %2")
                                       .arg(COMMAND_QMLPROPERTYGROUP).arg(qmlPropertyGroupTopic.args));
            }
            else {
                /*
                  Assumption: No '.' in the property name
                  means there is no property group.
                 */
                qmlPropertyGroupTopic.clear();
            }
        }
    }

    if (!qmlPropertyGroupTopic.isEmpty()) {
        arg = qmlPropertyGroupTopic.args;
        if (splitQmlPropertyGroupArg(arg, module, qmlType, property)) {
            qmlClass = qdb_->findQmlType(module, qmlType);
            if (qmlClass) {
                qpgn = new QmlPropertyGroupNode(qmlClass, property);
                qpgn->setLocation(doc.startLocation());
                nodes.append(qpgn);
                docs.append(doc);
            }
        }
    }
    for (int i=0; i<topics.size(); ++i) {
        if (topics.at(i).topic == COMMAND_QMLPROPERTYGROUP) {
             continue;
        }
        topic = topics.at(i).topic;
        arg = topics.at(i).args;
        if ((topic == COMMAND_QMLPROPERTY) || (topic == COMMAND_QMLATTACHEDPROPERTY)) {
            bool attached = (topic == COMMAND_QMLATTACHEDPROPERTY);
            if (splitQmlPropertyArg(arg, type, module, qmlType, property)) {
                qmlClass = qdb_->findQmlType(module, qmlType);
                if (qmlClass) {
                    if (qmlClass->hasQmlProperty(property) != 0) {
                        QString msg = tr("QML property documented multiple times: '%1'").arg(arg);
                        doc.startLocation().warning(msg);
                    }
                    else if (qpgn) {
                        qpn = new QmlPropertyNode(qpgn, property, type, attached);
                        qpn->setLocation(doc.startLocation());
                    }
                    else {
                        qpn = new QmlPropertyNode(qmlClass, property, type, attached);
                        qpn->setLocation(doc.startLocation());
                        nodes.append(qpn);
                        docs.append(doc);
                    }
                }
            }
        }
    }
}

static QSet<QString> otherMetaCommands_;
/*!
  Returns the set of strings representing the common metacommands
  plus some other metacommands.
 */
const QSet<QString>& CppCodeParser::otherMetaCommands()
{
    if (otherMetaCommands_.isEmpty()) {
        otherMetaCommands_ = commonMetaCommands();
        otherMetaCommands_ << COMMAND_INHEADERFILE
                           << COMMAND_OVERLOAD
                           << COMMAND_REIMP
                           << COMMAND_RELATES
                           << COMMAND_CONTENTSPAGE
                           << COMMAND_NEXTPAGE
                           << COMMAND_PREVIOUSPAGE
                           << COMMAND_INDEXPAGE
                           << COMMAND_STARTPAGE
                           << COMMAND_QMLINHERITS
                           << COMMAND_QMLINSTANTIATES
                           << COMMAND_QMLDEFAULT
                           << COMMAND_QMLREADONLY
                           << COMMAND_QMLABSTRACT;
    }
    return otherMetaCommands_;
}

/*!
  Process the metacommand \a command in the context of the
  \a node associated with the topic command and the \a doc.
  \a arg is the argument to the metacommand.
 */
void CppCodeParser::processOtherMetaCommand(const Doc& doc,
                                            const QString& command,
                                            const ArgLocPair& argLocPair,
                                            Node *node)
{
    QString arg = argLocPair.first;
    if (command == COMMAND_INHEADERFILE) {
        if (node != 0 && node->isInnerNode()) {
            ((InnerNode *) node)->addInclude(arg);
        }
        else {
            doc.location().warning(tr("Ignored '\\%1'")
                                   .arg(COMMAND_INHEADERFILE));
        }
    }
    else if (command == COMMAND_OVERLOAD) {
        if (node != 0 && node->type() == Node::Function) {
            ((FunctionNode *) node)->setOverload(true);
        }
        else {
            doc.location().warning(tr("Ignored '\\%1'")
                                   .arg(COMMAND_OVERLOAD));
        }
    }
    else if (command == COMMAND_REIMP) {
        if (node != 0 && node->parent() && !node->parent()->isInternal()) {
            if (node->type() == Node::Function) {
                FunctionNode *func = (FunctionNode *) node;
                const FunctionNode *from = func->reimplementedFrom();
                if (from == 0) {
                    doc.location().warning(tr("Cannot find base function for '\\%1' in %2()")
                                           .arg(COMMAND_REIMP).arg(node->name()),
                                           tr("The function either doesn't exist in any "
                                              "base class with the same signature or it "
                                              "exists but isn't virtual."));
                }
                /*
                  Ideally, we would enable this check to warn whenever
                  \reimp is used incorrectly, and only make the node
                  internal if the function is a reimplementation of
                  another function in a base class.
                */
                else if (from->access() == Node::Private
                         || from->parent()->access() == Node::Private) {
                    doc.location().warning(tr("'\\%1' in %2() should be '\\internal' "
                                              "because its base function is private "
                                              "or internal").arg(COMMAND_REIMP).arg(node->name()));
                }
                func->setReimp(true);
            }
            else {
                doc.location().warning(tr("Ignored '\\%1' in %2")
                                       .arg(COMMAND_REIMP)
                                       .arg(node->name()));
            }
        }
    }
    else if (command == COMMAND_RELATES) {
        /*
          Find the node that this node relates to.
         */
        Node* n = 0;
        if (arg.startsWith(QLatin1Char('<')) || arg.startsWith('"')) {
            /*
              It should be a header file, I think.
            */
            n = qdb_->findNodeByNameAndType(QStringList(arg), Node::Document, Node::NoSubType);
        }
        else {
            /*
              If it wasn't a file, it should be either a class or a namespace.
             */
            QStringList newPath = arg.split("::");
            n = qdb_->findClassNode(newPath);
            if (!n)
                n = qdb_->findNamespaceNode(newPath);
        }

        if (!n) {
            /*
              Didn't ind it. Error...
             */
            doc.location().warning(tr("Cannot find '%1' in '\\%2'").arg(arg).arg(COMMAND_RELATES));
        }
        else {
            /*
              Found it. This node relates to it.
             */
            node->setRelates(static_cast<InnerNode*>(n));
        }
    }
    else if (command == COMMAND_CONTENTSPAGE) {
        setLink(node, Node::ContentsLink, arg);
    }
    else if (command == COMMAND_NEXTPAGE) {
        setLink(node, Node::NextLink, arg);
    }
    else if (command == COMMAND_PREVIOUSPAGE) {
        setLink(node, Node::PreviousLink, arg);
    }
    else if (command == COMMAND_INDEXPAGE) {
        setLink(node, Node::IndexLink, arg);
    }
    else if (command == COMMAND_STARTPAGE) {
        setLink(node, Node::StartLink, arg);
    }
    else if (command == COMMAND_QMLINHERITS) {
        if (node->name() == arg)
            doc.location().warning(tr("%1 tries to inherit itself").arg(arg));
        else if (node->subType() == Node::QmlClass) {
            QmlClassNode *qmlClass = static_cast<QmlClassNode*>(node);
            qmlClass->setQmlBaseName(arg);
            QmlClassNode::addInheritedBy(arg,node);
        }
    }
    else if (command == COMMAND_QMLINSTANTIATES) {
        if ((node->type() == Node::Document) && (node->subType() == Node::QmlClass)) {
            ClassNode* classNode = qdb_->findClassNode(arg.split("::"));
            if (classNode)
                node->setClassNode(classNode);
            else
                doc.location().warning(tr("C++ class %1 not found: \\instantiates %1").arg(arg));
        }
        else
            doc.location().warning(tr("\\instantiates is only allowed in \\qmltype"));
    }
    else if (command == COMMAND_QMLDEFAULT) {
        if (node->type() == Node::QmlProperty) {
            QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
            qpn->setDefault();
        }
        else if (node->type() == Node::QmlPropertyGroup) {
            QmlPropertyGroupNode* qpgn = static_cast<QmlPropertyGroupNode*>(node);
            NodeList::ConstIterator p = qpgn->childNodes().constBegin();
            while (p != qpgn->childNodes().constEnd()) {
                if ((*p)->type() == Node::QmlProperty) {
                    QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(*p);
                    qpn->setDefault();
                }
                ++p;
            }
        }
    }
    else if (command == COMMAND_QMLREADONLY) {
        if (node->type() == Node::QmlProperty) {
            QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(node);
            qpn->setReadOnly(1);
        }
        else if (node->type() == Node::QmlPropertyGroup) {
            QmlPropertyGroupNode* qpgn = static_cast<QmlPropertyGroupNode*>(node);
            NodeList::ConstIterator p = qpgn->childNodes().constBegin();
            while (p != qpgn->childNodes().constEnd()) {
                if ((*p)->type() == Node::QmlProperty) {
                    QmlPropertyNode* qpn = static_cast<QmlPropertyNode*>(*p);
                    qpn->setReadOnly(1);
                }
                ++p;
            }
        }
    }
    else if (command == COMMAND_QMLABSTRACT) {
        if ((node->type() == Node::Document) && (node->subType() == Node::QmlClass)) {
            node->setAbstract(true);
        }
    }
    else {
        processCommonMetaCommand(doc.location(),command,argLocPair,node);
    }
}

/*!
  The topic command has been processed resulting in the \a doc
  and \a node passed in here. Process the other meta commands,
  which are found in \a doc, in the context of the topic \a node.
 */
void CppCodeParser::processOtherMetaCommands(const Doc& doc, Node *node)
{
    const QSet<QString> metaCommands = doc.metaCommandsUsed();
    QSet<QString>::ConstIterator cmd = metaCommands.constBegin();
    while (cmd != metaCommands.constEnd()) {
        ArgList args = doc.metaCommandArgs(*cmd);
        ArgList::ConstIterator arg = args.constBegin();
        while (arg != args.constEnd()) {
            processOtherMetaCommand(doc, *cmd, *arg, node);
            ++arg;
        }
        ++cmd;
    }
}

/*!
  Resets the C++ code parser to its default initialized state.
 */
void CppCodeParser::reset()
{
    tokenizer = 0;
    tok = 0;
    access = Node::Public;
    metaness = FunctionNode::Plain;
    lastPath_.clear();
    moduleName.clear();
}

/*!
  Get the next token from the file being parsed and store it
  in the token variable.
 */
void CppCodeParser::readToken()
{
    tok = tokenizer->getToken();
}

/*!
  Return the current location in the file being parsed,
  i.e. the file name, line number, and column number.
 */
const Location& CppCodeParser::location()
{
    return tokenizer->location();
}

/*!
  Return the previous string read from the file being parsed.
 */
QString CppCodeParser::previousLexeme()
{
    return tokenizer->previousLexeme();
}

/*!
  Return the current string string from the file being parsed.
 */
QString CppCodeParser::lexeme()
{
    return tokenizer->lexeme();
}

bool CppCodeParser::match(int target)
{
    if (tok == target) {
        readToken();
        return true;
    }
    else
        return false;
}

/*!
  Skip to \a target. If \a target is found before the end
  of input, return true. Otherwise return false.
 */
bool CppCodeParser::skipTo(int target)
{
    while ((tok != Tok_Eoi) && (tok != target))
        readToken();
    return (tok == target ? true : false);
}

/*!
  If the current token is one of the keyword thingees that
  are used in Qt, skip over it to the next token and return
  true. Otherwise just return false without reading the
  next token.
 */
bool CppCodeParser::matchCompat()
{
    switch (tok) {
    case Tok_QT_COMPAT:
    case Tok_QT_COMPAT_CONSTRUCTOR:
    case Tok_QT_DEPRECATED:
    case Tok_QT_MOC_COMPAT:
    case Tok_QT3_SUPPORT:
    case Tok_QT3_SUPPORT_CONSTRUCTOR:
    case Tok_QT3_MOC_SUPPORT:
        readToken();
        return true;
    default:
        return false;
    }
}

bool CppCodeParser::matchModuleQualifier(QString& name)
{
    bool matches = (lexeme() == QString('.'));
    if (matches) {
        do {
            name += lexeme();
            readToken();
        } while ((tok == Tok_Ident) || (lexeme() == QString('.')));
    }
    return matches;
}

bool CppCodeParser::matchTemplateAngles(CodeChunk *dataType)
{
    bool matches = (tok == Tok_LeftAngle);
    if (matches) {
        int leftAngleDepth = 0;
        int parenAndBraceDepth = 0;
        do {
            if (tok == Tok_LeftAngle) {
                leftAngleDepth++;
            }
            else if (tok == Tok_RightAngle) {
                leftAngleDepth--;
            }
            else if (tok == Tok_LeftParen || tok == Tok_LeftBrace) {
                ++parenAndBraceDepth;
            }
            else if (tok == Tok_RightParen || tok == Tok_RightBrace) {
                if (--parenAndBraceDepth < 0)
                    return false;
            }
            if (dataType != 0)
                dataType->append(lexeme());
            readToken();
        } while (leftAngleDepth > 0 && tok != Tok_Eoi);
    }
    return matches;
}

/*
  This function is no longer used.
 */
bool CppCodeParser::matchTemplateHeader()
{
    readToken();
    return matchTemplateAngles();
}

bool CppCodeParser::matchDataType(CodeChunk *dataType, QString *var)
{
    /*
      This code is really hard to follow... sorry. The loop is there to match
      Alpha::Beta::Gamma::...::Omega.
    */
    for (;;) {
        bool virgin = true;

        if (tok != Tok_Ident) {
            /*
              There is special processing for 'Foo::operator int()'
              and such elsewhere. This is the only case where we
              return something with a trailing gulbrandsen ('Foo::').
            */
            if (tok == Tok_operator)
                return true;

            /*
              People may write 'const unsigned short' or
              'short unsigned const' or any other permutation.
            */
            while (match(Tok_const) || match(Tok_volatile))
                dataType->append(previousLexeme());
            while (match(Tok_signed) || match(Tok_unsigned) ||
                   match(Tok_short) || match(Tok_long) || match(Tok_int64)) {
                dataType->append(previousLexeme());
                virgin = false;
            }
            while (match(Tok_const) || match(Tok_volatile))
                dataType->append(previousLexeme());

            if (match(Tok_Tilde))
                dataType->append(previousLexeme());
        }

        if (virgin) {
            if (match(Tok_Ident)) {
                /*
                  This is a hack until we replace this "parser"
                  with the real one used in Qt Creator.
                 */
                if (!inMacroCommand_ && lexeme() == "(" &&
                    ((previousLexeme() == "QT_PREPEND_NAMESPACE") || (previousLexeme() == "NS"))) {
                    readToken();
                    readToken();
                    dataType->append(previousLexeme());
                    readToken();
                }
                else
                    dataType->append(previousLexeme());
            }
            else if (match(Tok_void) || match(Tok_int) || match(Tok_char) ||
                     match(Tok_double) || match(Tok_Ellipsis))
                dataType->append(previousLexeme());
            else {
                return false;
            }
        }
        else if (match(Tok_int) || match(Tok_char) || match(Tok_double)) {
            dataType->append(previousLexeme());
        }

        matchTemplateAngles(dataType);

        while (match(Tok_const) || match(Tok_volatile))
            dataType->append(previousLexeme());

        if (match(Tok_Gulbrandsen))
            dataType->append(previousLexeme());
        else
            break;
    }

    while (match(Tok_Ampersand) || match(Tok_Aster) || match(Tok_const) ||
           match(Tok_Caret))
        dataType->append(previousLexeme());

    if (match(Tok_LeftParenAster)) {
        /*
          A function pointer. This would be rather hard to handle without a
          tokenizer hack, because a type can be followed with a left parenthesis
          in some cases (e.g., 'operator int()'). The tokenizer recognizes '(*'
          as a single token.
        */
        dataType->append(previousLexeme());
        dataType->appendHotspot();
        if (var != 0 && match(Tok_Ident))
            *var = previousLexeme();
        if (!match(Tok_RightParen) || tok != Tok_LeftParen) {
            return false;
        }
        dataType->append(previousLexeme());

        int parenDepth0 = tokenizer->parenDepth();
        while (tokenizer->parenDepth() >= parenDepth0 && tok != Tok_Eoi) {
            dataType->append(lexeme());
            readToken();
        }
        if (match(Tok_RightParen))
            dataType->append(previousLexeme());
    }
    else {
        /*
          The common case: Look for an optional identifier, then for
          some array brackets.
        */
        dataType->appendHotspot();

        if (var != 0) {
            if (match(Tok_Ident)) {
                *var = previousLexeme();
            }
            else if (match(Tok_Comment)) {
                /*
                  A neat hack: Commented-out parameter names are
                  recognized by qdoc. It's impossible to illustrate
                  here inside a C-style comment, because it requires
                  an asterslash. It's also impossible to illustrate
                  inside a C++-style comment, because the explanation
                  does not fit on one line.
                */
                if (varComment.exactMatch(previousLexeme()))
                    *var = varComment.cap(1);
            }
        }

        if (tok == Tok_LeftBracket) {
            int bracketDepth0 = tokenizer->bracketDepth();
            while ((tokenizer->bracketDepth() >= bracketDepth0 &&
                    tok != Tok_Eoi) ||
                   tok == Tok_RightBracket) {
                dataType->append(lexeme());
                readToken();
            }
        }
    }
    return true;
}

bool CppCodeParser::matchParameter(FunctionNode *func)
{
    CodeChunk dataType;
    QString name;
    CodeChunk defaultValue;

    if (!matchDataType(&dataType, &name)) {
        return false;
    }
    match(Tok_Comment);
    if (match(Tok_Equal)) {
        int parenDepth0 = tokenizer->parenDepth();

        while (tokenizer->parenDepth() >= parenDepth0 &&
               (tok != Tok_Comma ||
                tokenizer->parenDepth() > parenDepth0) &&
               tok != Tok_Eoi) {
            defaultValue.append(lexeme());
            readToken();
        }
    }
    func->addParameter(Parameter(dataType.toString(), "", name, defaultValue.toString())); // ###
    return true;
}

bool CppCodeParser::matchFunctionDecl(InnerNode *parent,
                                      QStringList *parentPathPtr,
                                      FunctionNode **funcPtr,
                                      const QString &templateStuff,
                                      ExtraFuncData& extra)
{
    CodeChunk returnType;
    QStringList parentPath;
    QString name;
    bool compat = false;

    if (match(Tok_friend)) {
        return false;
    }
    match(Tok_explicit);
    if (matchCompat())
        compat = true;
    bool sta = false;
    if (match(Tok_static)) {
        sta = true;
        if (matchCompat())
            compat = true;
    }
    FunctionNode::Virtualness vir = FunctionNode::NonVirtual;
    if (match(Tok_virtual)) {
        vir = FunctionNode::ImpureVirtual;
        if (matchCompat())
            compat = true;
    }

    if (!matchDataType(&returnType)) {
        if (tokenizer->parsingFnOrMacro()
                && (match(Tok_Q_DECLARE_FLAGS) ||
                    match(Tok_Q_PROPERTY) ||
                    match(Tok_Q_PRIVATE_PROPERTY)))
            returnType = CodeChunk(previousLexeme());
        else {
            return false;
        }
    }

    if (returnType.toString() == "QBool")
        returnType = CodeChunk("bool");

    if (matchCompat())
        compat = true;

    if (tok == Tok_operator &&
            (returnType.toString().isEmpty() ||
             returnType.toString().endsWith("::"))) {
        // 'QString::operator const char *()'
        parentPath = returnType.toString().split(sep);
        parentPath.removeAll(QString());
        returnType = CodeChunk();
        readToken();

        CodeChunk restOfName;
        if (tok != Tok_Tilde && matchDataType(&restOfName)) {
            name = "operator " + restOfName.toString();
        }
        else {
            name = previousLexeme() + lexeme();
            readToken();
            while (tok != Tok_LeftParen && tok != Tok_Eoi) {
                name += lexeme();
                readToken();
            }
        }
        if (tok != Tok_LeftParen) {
            return false;
        }
    }
    else if (tok == Tok_LeftParen) {
        // constructor or destructor
        parentPath = returnType.toString().split(sep);
        if (!parentPath.isEmpty()) {
            name = parentPath.last();
            parentPath.erase(parentPath.end() - 1);
        }
        returnType = CodeChunk();
    }
    else {
        while (match(Tok_Ident)) {
            name = previousLexeme();

            /*
              This is a hack to let QML module identifiers through.
             */
            matchModuleQualifier(name);

            matchTemplateAngles();

            if (match(Tok_Gulbrandsen))
                parentPath.append(name);
            else
                break;
        }

        if (tok == Tok_operator) {
            name = lexeme();
            readToken();
            while (tok != Tok_Eoi) {
                name += lexeme();
                readToken();
                if (tok == Tok_LeftParen)
                    break;
            }
        }
        if (parent && (tok == Tok_Semicolon ||
                       tok == Tok_LeftBracket ||
                       tok == Tok_Colon)
                && access != Node::Private) {
            if (tok == Tok_LeftBracket) {
                returnType.appendHotspot();

                int bracketDepth0 = tokenizer->bracketDepth();
                while ((tokenizer->bracketDepth() >= bracketDepth0 &&
                        tok != Tok_Eoi) ||
                       tok == Tok_RightBracket) {
                    returnType.append(lexeme());
                    readToken();
                }
                if (tok != Tok_Semicolon) {
                    return false;
                }
            }
            else if (tok == Tok_Colon) {
                returnType.appendHotspot();

                while (tok != Tok_Semicolon && tok != Tok_Eoi) {
                    returnType.append(lexeme());
                    readToken();
                }
                if (tok != Tok_Semicolon) {
                    return false;
                }
            }

            VariableNode *var = new VariableNode(parent, name);
            var->setAccess(access);
            var->setLocation(location());
            var->setLeftType(returnType.left());
            var->setRightType(returnType.right());
            if (compat)
                var->setStatus(Node::Compat);
            var->setStatic(sta);
            return false;
        }
        if (tok != Tok_LeftParen) {
            return false;
        }
    }
    readToken();

    FunctionNode *func = new FunctionNode(extra.type, parent, name, extra.isAttached);
    func->setAccess(access);
    func->setLocation(location());
    func->setReturnType(returnType.toString());
    func->setParentPath(parentPath);
    func->setTemplateStuff(templateStuff);
    if (compat)
        func->setStatus(Node::Compat);

    func->setMetaness(metaness);
    if (parent) {
        if (name == parent->name()) {
            func->setMetaness(FunctionNode::Ctor);
        } else if (name.startsWith(QLatin1Char('~')))  {
            func->setMetaness(FunctionNode::Dtor);
        }
    }
    func->setStatic(sta);

    if (tok != Tok_RightParen) {
        do {
            if (!matchParameter(func)) {
                return false;
            }
        } while (match(Tok_Comma));
    }
    if (!match(Tok_RightParen)) {
        return false;
    }

    func->setConst(match(Tok_const));

    if (match(Tok_Equal) && match(Tok_Number))
        vir = FunctionNode::PureVirtual;
    func->setVirtualness(vir);

    if (match(Tok_Colon)) {
        while (tok != Tok_LeftBrace && tok != Tok_Eoi)
            readToken();
    }

    if (!match(Tok_Semicolon) && tok != Tok_Eoi) {
        int braceDepth0 = tokenizer->braceDepth();

        if (!match(Tok_LeftBrace)) {
            return false;
        }
        while (tokenizer->braceDepth() >= braceDepth0 && tok != Tok_Eoi)
            readToken();
        match(Tok_RightBrace);
    }
    if (parentPathPtr != 0)
        *parentPathPtr = parentPath;
    if (funcPtr != 0)
        *funcPtr = func;
    return true;
}

bool CppCodeParser::matchBaseSpecifier(ClassNode *classe, bool isClass)
{
    Node::Access access;

    switch (tok) {
    case Tok_public:
        access = Node::Public;
        readToken();
        break;
    case Tok_protected:
        access = Node::Protected;
        readToken();
        break;
    case Tok_private:
        access = Node::Private;
        readToken();
        break;
    default:
        access = isClass ? Node::Private : Node::Public;
    }

    if (tok == Tok_virtual)
        readToken();

    CodeChunk baseClass;
    if (!matchDataType(&baseClass))
        return false;

    qdb_->addBaseClass(classe,
                       access,
                       baseClass.toPath(),
                       baseClass.toString(),
                       classe->parent());
    return true;
}

bool CppCodeParser::matchBaseList(ClassNode *classe, bool isClass)
{
    for (;;) {
        if (!matchBaseSpecifier(classe, isClass))
            return false;
        if (tok == Tok_LeftBrace)
            return true;
        if (!match(Tok_Comma))
            return false;
    }
}

/*!
  Parse a C++ class, union, or struct declaration.

  This function only handles one level of class nesting, but that is
  sufficient for Qt because there are no cases of class nesting more
  than one level deep.
 */
bool CppCodeParser::matchClassDecl(InnerNode *parent,
                                   const QString &templateStuff)
{
    bool isClass = (tok == Tok_class);
    readToken();

    bool compat = matchCompat();

    if (tok != Tok_Ident)
        return false;
    while (tok == Tok_Ident)
        readToken();
    if (tok == Tok_Gulbrandsen) {
        Node* n = parent->findChildNodeByNameAndType(previousLexeme(),Node::Class);
        if (n) {
            parent = static_cast<InnerNode*>(n);
            if (parent) {
                readToken();
                if (tok != Tok_Ident)
                    return false;
                readToken();
            }
        }
    }
    if (tok != Tok_Colon && tok != Tok_LeftBrace)
        return false;

    /*
      So far, so good. We have 'class Foo {' or 'class Foo :'.
      This is enough to recognize a class definition.
    */
    ClassNode *classe = new ClassNode(parent, previousLexeme());
    classe->setAccess(access);
    classe->setLocation(location());
    if (compat)
        classe->setStatus(Node::Compat);
    if (!moduleName.isEmpty())
        classe->setModuleName(moduleName);
    classe->setTemplateStuff(templateStuff);

    if (match(Tok_Colon) && !matchBaseList(classe, isClass))
        return false;
    if (!match(Tok_LeftBrace))
        return false;

    Node::Access outerAccess = access;
    access = isClass ? Node::Private : Node::Public;
    FunctionNode::Metaness outerMetaness = metaness;
    metaness = FunctionNode::Plain;

    bool matches = (matchDeclList(classe) && match(Tok_RightBrace) &&
                    match(Tok_Semicolon));
    access = outerAccess;
    metaness = outerMetaness;
    return matches;
}

bool CppCodeParser::matchNamespaceDecl(InnerNode *parent)
{
    readToken(); // skip 'namespace'
    if (tok != Tok_Ident)
        return false;
    while (tok == Tok_Ident)
        readToken();
    if (tok != Tok_LeftBrace)
        return false;

    /*
        So far, so good. We have 'namespace Foo {'.
    */
    QString namespaceName = previousLexeme();
    NamespaceNode* ns = 0;
    if (parent) {
        ns = static_cast<NamespaceNode*>(parent->findChildNodeByNameAndType(namespaceName, Node::Namespace));
    }
    if (!ns) {
        ns = new NamespaceNode(parent, namespaceName);
        ns->setAccess(access);
        ns->setLocation(location());
    }

    readToken(); // skip '{'
    bool matched = matchDeclList(ns);
    return matched && match(Tok_RightBrace);
}

bool CppCodeParser::matchUsingDecl()
{
    readToken(); // skip 'using'

    // 'namespace'
    if (tok != Tok_namespace)
        return false;

    readToken();
    // identifier
    if (tok != Tok_Ident)
        return false;

    QString name;
    while (tok == Tok_Ident) {
        name += lexeme();
        readToken();
        if (tok == Tok_Semicolon)
            break;
        else if (tok != Tok_Gulbrandsen)
            return false;
        name += "::";
        readToken();
    }

    /*
        So far, so good. We have 'using namespace Foo;'.
    */
    qdb_->insertOpenNamespace(name);
    return true;
}

bool CppCodeParser::matchEnumItem(InnerNode *parent, EnumNode *enume)
{
    if (!match(Tok_Ident))
        return false;

    QString name = previousLexeme();
    CodeChunk val;

    if (match(Tok_Equal)) {
        while (tok != Tok_Comma && tok != Tok_RightBrace &&
               tok != Tok_Eoi) {
            val.append(lexeme());
            readToken();
        }
    }

    if (enume) {
        QString strVal = val.toString();
        if (strVal.isEmpty()) {
            if (enume->items().isEmpty()) {
                strVal = "0";
            }
            else {
                QString last = enume->items().last().value();
                bool ok;
                int n = last.toInt(&ok);
                if (ok) {
                    if (last.startsWith(QLatin1Char('0')) && last.size() > 1) {
                        if (last.startsWith("0x") || last.startsWith("0X"))
                            strVal = last.left(2) + QString::number(n + 1, 16);
                        else
                            strVal = QLatin1Char('0') + QString::number(n + 1, 8);
                    }
                    else
                        strVal = QString::number(n + 1);
                }
            }
        }

        enume->addItem(EnumItem(name, strVal));
    }
    else {
        VariableNode *var = new VariableNode(parent, name);
        var->setAccess(access);
        var->setLocation(location());
        var->setLeftType("const int");
        var->setStatic(true);
    }
    return true;
}

bool CppCodeParser::matchEnumDecl(InnerNode *parent)
{
    QString name;

    if (!match(Tok_enum))
        return false;
    if (match(Tok_Ident))
        name = previousLexeme();
    if (tok != Tok_LeftBrace)
        return false;

    EnumNode *enume = 0;

    if (!name.isEmpty()) {
        enume = new EnumNode(parent, name);
        enume->setAccess(access);
        enume->setLocation(location());
    }

    readToken();

    if (!matchEnumItem(parent, enume))
        return false;

    while (match(Tok_Comma)) {
        if (!matchEnumItem(parent, enume))
            return false;
    }
    return match(Tok_RightBrace) && match(Tok_Semicolon);
}

bool CppCodeParser::matchTypedefDecl(InnerNode *parent)
{
    CodeChunk dataType;
    QString name;

    if (!match(Tok_typedef))
        return false;
    if (!matchDataType(&dataType, &name))
        return false;
    if (!match(Tok_Semicolon))
        return false;

    if (parent && !parent->findChildNodeByNameAndType(name, Node::Typedef)) {
        TypedefNode* td = new TypedefNode(parent, name);
        td->setAccess(access);
        td->setLocation(location());
    }
    return true;
}

bool CppCodeParser::matchProperty(InnerNode *parent)
{
    int expected_tok = Tok_LeftParen;
    if (match(Tok_Q_PRIVATE_PROPERTY)) {
        expected_tok = Tok_Comma;
        if (!skipTo(Tok_Comma))
            return false;
    }
    else if (!match(Tok_Q_PROPERTY) &&
             !match(Tok_Q_OVERRIDE) &&
             !match(Tok_QDOC_PROPERTY)) {
        return false;
    }

    if (!match(expected_tok))
        return false;

    QString name;
    CodeChunk dataType;
    if (!matchDataType(&dataType, &name))
        return false;

    PropertyNode *property = new PropertyNode(parent, name);
    property->setAccess(Node::Public);
    property->setLocation(location());
    property->setDataType(dataType.toString());

    while (tok != Tok_RightParen && tok != Tok_Eoi) {
        if (!match(Tok_Ident))
            return false;
        QString key = previousLexeme();
        QString value;

        // Keywords with no associated values
        if (key == "CONSTANT") {
            property->setConstant();
            continue;
        }
        else if (key == "FINAL") {
            property->setFinal();
            continue;
        }

        if (match(Tok_Ident) || match(Tok_Number)) {
            value = previousLexeme();
        }
        else if (match(Tok_LeftParen)) {
            int depth = 1;
            while (tok != Tok_Eoi) {
                if (tok == Tok_LeftParen) {
                    readToken();
                    ++depth;
                } else if (tok == Tok_RightParen) {
                    readToken();
                    if (--depth == 0)
                        break;
                } else {
                    readToken();
                }
            }
            value = "?";
        }

        if (key == "READ")
            qdb_->addPropertyFunction(property, value, PropertyNode::Getter);
        else if (key == "WRITE") {
            qdb_->addPropertyFunction(property, value, PropertyNode::Setter);
            property->setWritable(true);
        }
        else if (key == "STORED")
            property->setStored(value.toLower() == "true");
        else if (key == "DESIGNABLE") {
            QString v = value.toLower();
            if (v == "true")
                property->setDesignable(true);
            else if (v == "false")
                property->setDesignable(false);
            else {
                property->setDesignable(false);
                property->setRuntimeDesFunc(value);
            }
        }
        else if (key == "RESET")
            qdb_->addPropertyFunction(property, value, PropertyNode::Resetter);
        else if (key == "NOTIFY") {
            qdb_->addPropertyFunction(property, value, PropertyNode::Notifier);
        } else if (key == "REVISION") {
            int revision;
            bool ok;
            revision = value.toInt(&ok);
            if (ok)
                property->setRevision(revision);
            else
                location().warning(tr("Invalid revision number: %1").arg(value));
        } else if (key == "SCRIPTABLE") {
            QString v = value.toLower();
            if (v == "true")
                property->setScriptable(true);
            else if (v == "false")
                property->setScriptable(false);
            else {
                property->setScriptable(false);
                property->setRuntimeScrFunc(value);
            }
        }
    }
    match(Tok_RightParen);
    return true;
}

/*!
  Parse a C++ declaration.
 */
bool CppCodeParser::matchDeclList(InnerNode *parent)
{
    ExtraFuncData extra;
    QString templateStuff;
    int braceDepth0 = tokenizer->braceDepth();
    if (tok == Tok_RightBrace) // prevents failure on empty body
        braceDepth0++;

    while (tokenizer->braceDepth() >= braceDepth0 && tok != Tok_Eoi) {
        switch (tok) {
        case Tok_Colon:
            readToken();
            break;
        case Tok_class:
        case Tok_struct:
        case Tok_union:
            matchClassDecl(parent, templateStuff);
            break;
        case Tok_namespace:
            matchNamespaceDecl(parent);
            break;
        case Tok_using:
            matchUsingDecl();
            break;
        case Tok_template:
            {
                CodeChunk dataType;
                readToken();
                matchTemplateAngles(&dataType);
                templateStuff = dataType.toString();
            }
            continue;
        case Tok_enum:
            matchEnumDecl(parent);
            break;
        case Tok_typedef:
            matchTypedefDecl(parent);
            break;
        case Tok_private:
            readToken();
            access = Node::Private;
            metaness = FunctionNode::Plain;
            break;
        case Tok_protected:
            readToken();
            access = Node::Protected;
            metaness = FunctionNode::Plain;
            break;
        case Tok_public:
            readToken();
            access = Node::Public;
            metaness = FunctionNode::Plain;
            break;
        case Tok_signals:
        case Tok_Q_SIGNALS:
            readToken();
            access = Node::Public;
            metaness = FunctionNode::Signal;
            break;
        case Tok_slots:
        case Tok_Q_SLOTS:
            readToken();
            metaness = FunctionNode::Slot;
            break;
        case Tok_Q_OBJECT:
            readToken();
            break;
        case Tok_Q_OVERRIDE:
        case Tok_Q_PROPERTY:
        case Tok_Q_PRIVATE_PROPERTY:
        case Tok_QDOC_PROPERTY:
            if (!matchProperty(parent)) {
                location().warning(tr("Failed to parse token %1 in property declaration").arg(lexeme()));
                skipTo(Tok_RightParen);
                match(Tok_RightParen);
            }
            break;
        case Tok_Q_DECLARE_SEQUENTIAL_ITERATOR:
            readToken();
            if (match(Tok_LeftParen) && match(Tok_Ident))
                sequentialIteratorClasses.insert(previousLexeme(),
                                                 location().fileName());
            match(Tok_RightParen);
            break;
        case Tok_Q_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR:
            readToken();
            if (match(Tok_LeftParen) && match(Tok_Ident))
                mutableSequentialIteratorClasses.insert(previousLexeme(),
                                                        location().fileName());
            match(Tok_RightParen);
            break;
        case Tok_Q_DECLARE_ASSOCIATIVE_ITERATOR:
            readToken();
            if (match(Tok_LeftParen) && match(Tok_Ident))
                associativeIteratorClasses.insert(previousLexeme(),
                                                  location().fileName());
            match(Tok_RightParen);
            break;
        case Tok_Q_DECLARE_MUTABLE_ASSOCIATIVE_ITERATOR:
            readToken();
            if (match(Tok_LeftParen) && match(Tok_Ident))
                mutableAssociativeIteratorClasses.insert(previousLexeme(),
                                                         location().fileName());
            match(Tok_RightParen);
            break;
        case Tok_Q_DECLARE_FLAGS:
            readToken();
            if (match(Tok_LeftParen) && match(Tok_Ident)) {
                QString flagsType = previousLexeme();
                if (match(Tok_Comma) && match(Tok_Ident)) {
                    QString name = previousLexeme();
                    TypedefNode *flagsNode = new TypedefNode(parent, flagsType);
                    flagsNode->setAccess(access);
                    flagsNode->setLocation(location());
                    EnumNode* en = static_cast<EnumNode*>(parent->findChildNodeByNameAndType(name, Node::Enum));
                    if (en)
                        en->setFlagsType(flagsNode);
                }
            }
            match(Tok_RightParen);
            break;
        case Tok_QT_MODULE:
            readToken();
            if (match(Tok_LeftParen) && match(Tok_Ident))
                moduleName = previousLexeme();
            if (!moduleName.startsWith("Qt"))
                moduleName.prepend("Qt");
            match(Tok_RightParen);
            break;
        default:
            if (!matchFunctionDecl(parent, 0, 0, templateStuff, extra)) {
                while (tok != Tok_Eoi &&
                       (tokenizer->braceDepth() > braceDepth0 ||
                        (!match(Tok_Semicolon) &&
                         tok != Tok_public && tok != Tok_protected &&
                         tok != Tok_private))) {
                    readToken();
                }
            }
        }
        templateStuff.clear();
    }
    return true;
}

/*!
  This is called by parseSourceFile() to do the actual parsing
  and tree building.
 */
bool CppCodeParser::matchDocsAndStuff()
{
    ExtraFuncData extra;
    const QSet<QString>& topicCommandsAllowed = topicCommands();
    const QSet<QString>& otherMetacommandsAllowed = otherMetaCommands();
    const QSet<QString>& metacommandsAllowed = topicCommandsAllowed + otherMetacommandsAllowed;

    while (tok != Tok_Eoi) {
        if (tok == Tok_Doc) {
            /*
              lexeme() returns an entire qdoc comment.
             */
            QString comment = lexeme();
            Location start_loc(location());
            readToken();

            Doc::trimCStyleComment(start_loc,comment);
            Location end_loc(location());

            /*
              Doc parses the comment.
             */
            Doc doc(start_loc,end_loc,comment,metacommandsAllowed, topicCommandsAllowed);
            QString topic;
            bool isQmlPropertyTopic = false;

            const TopicList& topics = doc.topicsUsed();
            if (!topics.isEmpty()) {
                topic = topics[0].topic;
                if ((topic == COMMAND_QMLPROPERTY) ||
                    (topic == COMMAND_QMLPROPERTYGROUP) ||
                    (topic == COMMAND_QMLATTACHEDPROPERTY)) {
                    isQmlPropertyTopic = true;
                }
            }
            NodeList nodes;
            DocList docs;

            if (topic.isEmpty()) {
                QStringList parentPath;
                FunctionNode *clone;
                FunctionNode *func = 0;

                if (matchFunctionDecl(0, &parentPath, &clone, QString(), extra)) {
                    func = qdb_->findNodeInOpenNamespace(parentPath, clone);
                    if (func == 0)
                        func = qdb_->findFunctionNode(parentPath, clone);

                    if (func) {
                        func->borrowParameterNames(clone);
                        nodes.append(func);
                        docs.append(doc);
                    }
                    delete clone;
                }
                else {
                    doc.location().warning(tr("Cannot tie this documentation to anything"),
                                tr("I found a /*! ... */ comment, but there was no "
                                   "topic command (e.g., '\\%1', '\\%2') in the "
                                   "comment and no function definition following "
                                   "the comment.")
                                .arg(COMMAND_FN).arg(COMMAND_PAGE));
                }
            }
            else if (isQmlPropertyTopic) {
                Doc nodeDoc = doc;
                processQmlProperties(nodeDoc, nodes, docs);
            }
            else {
                ArgList args;
                const QSet<QString>& topicCommandsUsed = topicCommandsAllowed & doc.metaCommandsUsed();
                if (topicCommandsUsed.count() > 0) {
                    topic = *topicCommandsUsed.constBegin();
                    args = doc.metaCommandArgs(topic);
                }
                if (topicCommandsUsed.count() > 1) {
                    QString topics;
                    QSet<QString>::ConstIterator t = topicCommandsUsed.constBegin();
                    while (t != topicCommandsUsed.constEnd()) {
                        topics += " \\" + *t + ",";
                        ++t;
                    }
                    topics[topics.lastIndexOf(',')] = '.';
                    int i = topics.lastIndexOf(',');
                    topics[i] = ' ';
                    topics.insert(i+1,"and");
                    doc.location().warning(tr("Multiple topic commands found in comment: %1").arg(topics));
                }
                ArgList::ConstIterator a = args.constBegin();
                while (a != args.constEnd()) {
                    Doc nodeDoc = doc;
                    Node *node = processTopicCommand(nodeDoc,topic,*a);
                    if (node != 0) {
                        nodes.append(node);
                        docs.append(nodeDoc);
                    }
                    ++a;
                }
            }

            NodeList::Iterator n = nodes.begin();
            QList<Doc>::Iterator d = docs.begin();
            while (n != nodes.end()) {
                processOtherMetaCommands(*d, *n);
                (*n)->setDoc(*d);
                checkModuleInclusion(*n);
                if ((*n)->isInnerNode() && ((InnerNode *)*n)->includes().isEmpty()) {
                    InnerNode *m = static_cast<InnerNode *>(*n);
                    while (m->parent() != qdb_->treeRoot())
                        m = m->parent();
                    if (m == *n)
                        ((InnerNode *)*n)->addInclude((*n)->name());
                    else
                        ((InnerNode *)*n)->setIncludes(m->includes());
                }
                ++d;
                ++n;
            }
        }
        else if (tok == Tok_using) {
            matchUsingDecl();
        }
        else {
            QStringList parentPath;
            FunctionNode *clone;
            FunctionNode *node = 0;

            if (matchFunctionDecl(0, &parentPath, &clone, QString(), extra)) {
                /*
                  The location of the definition is more interesting
                  than that of the declaration. People equipped with
                  a sophisticated text editor can respond to warnings
                  concerning undocumented functions very quickly.

                  Signals are implemented in uninteresting files
                  generated by moc.
                */
                node = qdb_->findFunctionNode(parentPath, clone);
                if (node != 0 && node->metaness() != FunctionNode::Signal)
                    node->setLocation(clone->location());
                delete clone;
            }
            else {
                if (tok != Tok_Doc)
                    readToken();
            }
        }
    }
    return true;
}

/*!
  This function uses a Tokenizer to parse the function \a signature
  in an attempt to match it to the signature of a child node of \a root.
  If a match is found, \a funcPtr is set to point to the matching node
  and true is returned.
 */
bool CppCodeParser::makeFunctionNode(const QString& signature,
                                     QStringList* parentPathPtr,
                                     FunctionNode** funcPtr,
                                     ExtraFuncData& extra)
{
    Tokenizer* outerTokenizer = tokenizer;
    int outerTok = tok;

    QByteArray latin1 = signature.toLatin1();
    Tokenizer stringTokenizer(location(), latin1);
    stringTokenizer.setParsingFnOrMacro(true);
    tokenizer = &stringTokenizer;
    readToken();

    inMacroCommand_ = extra.isMacro;
    bool ok = matchFunctionDecl(extra.root, parentPathPtr, funcPtr, QString(), extra);
    inMacroCommand_ = false;
    // potential memory leak with funcPtr

    tokenizer = outerTokenizer;
    tok = outerTok;
    return ok;
}

/*!
  Create a new FunctionNode for a QML method or signal, as
  specified by \a type, as a child of \a parent. \a sig is
  the complete signature, and if \a attached is true, the
  method or signal is "attached". \a qdoctag is the text of
  the \a type.

  \a parent is the QML class node. The QML module and QML
  type names have already been consumed to find \a parent.
  What remains in \a sig is the method signature. The method
  must be a child of \a parent.
 */
FunctionNode* CppCodeParser::makeFunctionNode(const Doc& doc,
                                              const QString& sig,
                                              InnerNode* parent,
                                              Node::Type type,
                                              bool attached,
                                              QString qdoctag)
{
    QStringList pp;
    FunctionNode* fn = 0;
    ExtraFuncData extra(parent, type, attached);
    if (!makeFunctionNode(sig, &pp, &fn, extra) && !makeFunctionNode("void " + sig, &pp, &fn, extra)) {
        doc.location().warning(tr("Invalid syntax in '\\%1'").arg(qdoctag));
    }
    return fn;
}

void CppCodeParser::parseQiteratorDotH(const Location &location, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
        return;

    QString text = file.readAll();
    text.remove("\r");
    text.remove("\\\n");
    QStringList lines = text.split(QLatin1Char('\n'));
    lines = lines.filter("Q_DECLARE");
    lines.replaceInStrings(QRegExp("#define Q[A-Z_]*\\(C\\)"), QString());

    if (lines.size() == 4) {
        sequentialIteratorDefinition = lines[0];
        mutableSequentialIteratorDefinition = lines[1];
        associativeIteratorDefinition = lines[2];
        mutableAssociativeIteratorDefinition = lines[3];
    }
    else {
        location.warning(tr("The qiterator.h hack failed"));
    }
}

void CppCodeParser::instantiateIteratorMacro(const QString &container,
                                             const QString &includeFile,
                                             const QString &macroDef)
{
    QString resultingCode = macroDef;
    resultingCode.replace(QRegExp("\\bC\\b"), container);
    resultingCode.remove(QRegExp("\\s*##\\s*"));

    Location loc(includeFile);   // hack to get the include file for free
    QByteArray latin1 = resultingCode.toLatin1();
    Tokenizer stringTokenizer(loc, latin1);
    tokenizer = &stringTokenizer;
    readToken();
    matchDeclList(QDocDatabase::qdocDB()->treeRoot());
}

void CppCodeParser::createExampleFileNodes(DocNode *dn)
{
    QString examplePath = dn->name();
    QString proFileName = examplePath + QLatin1Char('/') + examplePath.split(QLatin1Char('/')).last() + ".pro";
    QString userFriendlyFilePath;

    QString fullPath = Config::findFile(dn->doc().location(),
                                        exampleFiles,
                                        exampleDirs,
                                        proFileName,
                                        userFriendlyFilePath);

    if (fullPath.isEmpty()) {
        QString tmp = proFileName;
        proFileName = examplePath + QLatin1Char('/') + "qbuild.pro";
        userFriendlyFilePath.clear();
        fullPath = Config::findFile(dn->doc().location(),
                                    exampleFiles,
                                    exampleDirs,
                                    proFileName,
                                    userFriendlyFilePath);
        if (fullPath.isEmpty()) {
            proFileName = examplePath + QLatin1Char('/') + examplePath.split(QLatin1Char('/')).last() + ".qmlproject";
            userFriendlyFilePath.clear();
            fullPath = Config::findFile(dn->doc().location(),
                                        exampleFiles,
                                        exampleDirs,
                                        proFileName,
                                        userFriendlyFilePath);
            if (fullPath.isEmpty()) {
                dn->location().warning(tr("Cannot find file '%1' or '%2'").arg(tmp).arg(proFileName));
                dn->location().warning(tr("  EXAMPLE PATH DOES NOT EXIST: %1").arg(examplePath));
                return;
            }
        }
    }

    int sizeOfBoringPartOfName = fullPath.size() - proFileName.size();
    if (fullPath.startsWith("./"))
        sizeOfBoringPartOfName = sizeOfBoringPartOfName - 2;
    fullPath.truncate(fullPath.lastIndexOf('/'));

    QStringList exampleFiles = Config::getFilesHere(fullPath,exampleNameFilter);
    QString imagesPath = fullPath + "/images";
    QStringList imageFiles = Config::getFilesHere(imagesPath,exampleImageFilter);
    if (!exampleFiles.isEmpty()) {
        // move main.cpp and to the end, if it exists
        QString mainCpp;
        QMutableStringListIterator i(exampleFiles);
        i.toBack();
        while (i.hasPrevious()) {
            QString fileName = i.previous();
            if (fileName.endsWith("/main.cpp")) {
                mainCpp = fileName;
                i.remove();
            }
            else if (fileName.contains("/qrc_") || fileName.contains("/moc_")
                     || fileName.contains("/ui_"))
                i.remove();
        }
        if (!mainCpp.isEmpty())
            exampleFiles.append(mainCpp);

        // add any qmake Qt resource files and qmake project files
        exampleFiles += Config::getFilesHere(fullPath, "*.qrc *.pro *.qmlproject qmldir");
    }

    foreach (const QString &exampleFile, exampleFiles) {
        new DocNode(dn,
                    exampleFile.mid(sizeOfBoringPartOfName),
                    Node::File,
                    Node::NoPageType);
    }
    foreach (const QString &imageFile, imageFiles) {
        new DocNode(dn,
                    imageFile.mid(sizeOfBoringPartOfName),
                    Node::Image,
                    Node::NoPageType);
    }
}

QT_END_NAMESPACE
