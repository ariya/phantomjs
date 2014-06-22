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

#ifndef CPPCODEPARSER_H
#define CPPCODEPARSER_H

#include <qregexp.h>

#include "codeparser.h"

QT_BEGIN_NAMESPACE

class ClassNode;
class CodeChunk;
class CppCodeParserPrivate;
class FunctionNode;
class InnerNode;
class Tokenizer;

class CppCodeParser : public CodeParser
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::CppCodeParser)

    struct ExtraFuncData {
        InnerNode* root; // Used as the parent.
        Node::Type type; // The node type: Function, etc.
        bool isAttached; // If true, the method is attached.
        bool isMacro;    // If true, we are parsing a macro signature.
        ExtraFuncData() : root(0), type(Node::Function), isAttached(false), isMacro(false) { }
        ExtraFuncData(InnerNode* r, Node::Type t, bool a)
          : root(r), type(t), isAttached(a), isMacro(false) { }
    };

public:
    CppCodeParser();
    ~CppCodeParser();

    virtual void initializeParser(const Config& config);
    virtual void terminateParser();
    virtual QString language();
    virtual QStringList headerFileNameFilter();
    virtual QStringList sourceFileNameFilter();
    virtual void parseHeaderFile(const Location& location, const QString& filePath);
    virtual void parseSourceFile(const Location& location, const QString& filePath);
    virtual void doneParsingHeaderFiles();
    virtual void doneParsingSourceFiles();

protected:
    const QSet<QString>& topicCommands();
    const QSet<QString>& otherMetaCommands();
    virtual Node* processTopicCommand(const Doc& doc,
                                      const QString& command,
                                      const ArgLocPair& arg);
    void processQmlProperties(const Doc& doc, NodeList& nodes, DocList& docs);
    bool splitQmlPropertyGroupArg(const QString& arg,
                                  QString& module,
                                  QString& element,
                                  QString& name);
    bool splitQmlPropertyArg(const QString& arg,
                             QString& type,
                             QString& module,
                             QString& element,
                             QString& name);
    bool splitQmlMethodArg(const QString& arg,
                           QString& type,
                           QString& module,
                           QString& element);
    virtual void processOtherMetaCommand(const Doc& doc,
                                         const QString& command,
                                         const ArgLocPair& argLocPair,
                                         Node *node);
    void processOtherMetaCommands(const Doc& doc, Node *node);

 protected:
    void reset();
    void readToken();
    const Location& location();
    QString previousLexeme();
    QString lexeme();

 private:
    bool match(int target);
    bool skipTo(int target);
    bool matchCompat();
    bool matchModuleQualifier(QString& name);
    bool matchTemplateAngles(CodeChunk *type = 0);
    bool matchTemplateHeader();
    bool matchDataType(CodeChunk *type, QString *var = 0);
    bool matchParameter(FunctionNode *func);
    bool matchFunctionDecl(InnerNode *parent,
                           QStringList *parentPathPtr,
                           FunctionNode **funcPtr,
                           const QString &templateStuff,
                           ExtraFuncData& extra);
    bool matchBaseSpecifier(ClassNode *classe, bool isClass);
    bool matchBaseList(ClassNode *classe, bool isClass);
    bool matchClassDecl(InnerNode *parent,
                        const QString &templateStuff = QString());
    bool matchNamespaceDecl(InnerNode *parent);
    bool matchUsingDecl();
    bool matchEnumItem(InnerNode *parent, EnumNode *enume);
    bool matchEnumDecl(InnerNode *parent);
    bool matchTypedefDecl(InnerNode *parent);
    bool matchProperty(InnerNode *parent);
    bool matchDeclList(InnerNode *parent);
    bool matchDocsAndStuff();
    bool makeFunctionNode(const QString &synopsis,
                          QStringList *parentPathPtr,
                          FunctionNode **funcPtr,
                          ExtraFuncData& params);
    FunctionNode* makeFunctionNode(const Doc& doc,
                                   const QString& sig,
                                   InnerNode* parent,
                                   Node::Type type,
                                   bool attached,
                                   QString qdoctag);
    void parseQiteratorDotH(const Location &location, const QString &filePath);
    void instantiateIteratorMacro(const QString &container,
                                  const QString &includeFile,
                                  const QString &macroDef);
    void createExampleFileNodes(DocNode *dn);

 protected:
    QMap<QString, Node::Type> nodeTypeMap;
    Tokenizer *tokenizer;
    int tok;
    Node::Access access;
    FunctionNode::Metaness metaness;
    QString moduleName;
    QStringList lastPath_;
    QRegExp varComment;
    QRegExp sep;

 private:
    QString sequentialIteratorDefinition;
    QString mutableSequentialIteratorDefinition;
    QString associativeIteratorDefinition;
    QString mutableAssociativeIteratorDefinition;
    QMap<QString, QString> sequentialIteratorClasses;
    QMap<QString, QString> mutableSequentialIteratorClasses;
    QMap<QString, QString> associativeIteratorClasses;
    QMap<QString, QString> mutableAssociativeIteratorClasses;

    static QStringList exampleFiles;
    static QStringList exampleDirs;
    QString exampleNameFilter;
    QString exampleImageFilter;
};

#define COMMAND_CLASS                   Doc::alias("class")
#define COMMAND_CONTENTSPAGE            Doc::alias("contentspage")
#define COMMAND_DITAMAP                 Doc::alias("ditamap")
#define COMMAND_ENUM                    Doc::alias("enum")
#define COMMAND_EXAMPLE                 Doc::alias("example")
#define COMMAND_EXTERNALPAGE            Doc::alias("externalpage")
#define COMMAND_FILE                    Doc::alias("file")
#define COMMAND_FN                      Doc::alias("fn")
#define COMMAND_GROUP                   Doc::alias("group")
#define COMMAND_HEADERFILE              Doc::alias("headerfile")
#define COMMAND_INDEXPAGE               Doc::alias("indexpage")
#define COMMAND_INHEADERFILE            Doc::alias("inheaderfile")
#define COMMAND_MACRO                   Doc::alias("macro")
#define COMMAND_MODULE                  Doc::alias("module")
#define COMMAND_NAMESPACE               Doc::alias("namespace")
#define COMMAND_OVERLOAD                Doc::alias("overload")
#define COMMAND_NEXTPAGE                Doc::alias("nextpage")
#define COMMAND_PAGE                    Doc::alias("page")
#define COMMAND_PREVIOUSPAGE            Doc::alias("previouspage")
#define COMMAND_PROPERTY                Doc::alias("property")
#define COMMAND_REIMP                   Doc::alias("reimp")
#define COMMAND_RELATES                 Doc::alias("relates")
#define COMMAND_STARTPAGE               Doc::alias("startpage")
#define COMMAND_TYPEDEF                 Doc::alias("typedef")
#define COMMAND_VARIABLE                Doc::alias("variable")
#define COMMAND_QMLABSTRACT             Doc::alias("qmlabstract")
#define COMMAND_QMLCLASS                Doc::alias("qmlclass")
#define COMMAND_QMLTYPE                 Doc::alias("qmltype")
#define COMMAND_QMLPROPERTY             Doc::alias("qmlproperty")
#define COMMAND_QMLPROPERTYGROUP        Doc::alias("qmlpropertygroup")
#define COMMAND_QMLATTACHEDPROPERTY     Doc::alias("qmlattachedproperty")
#define COMMAND_QMLINHERITS             Doc::alias("inherits")
#define COMMAND_QMLINSTANTIATES         Doc::alias("instantiates")
#define COMMAND_QMLSIGNAL               Doc::alias("qmlsignal")
#define COMMAND_QMLATTACHEDSIGNAL       Doc::alias("qmlattachedsignal")
#define COMMAND_QMLMETHOD               Doc::alias("qmlmethod")
#define COMMAND_QMLATTACHEDMETHOD       Doc::alias("qmlattachedmethod")
#define COMMAND_QMLDEFAULT              Doc::alias("default")
#define COMMAND_QMLREADONLY             Doc::alias("readonly")
#define COMMAND_QMLBASICTYPE            Doc::alias("qmlbasictype")
#define COMMAND_QMLMODULE               Doc::alias("qmlmodule")
#define COMMAND_AUDIENCE                Doc::alias("audience")
#define COMMAND_CATEGORY                Doc::alias("category")
#define COMMAND_PRODNAME                Doc::alias("prodname")
#define COMMAND_COMPONENT               Doc::alias("component")
#define COMMAND_AUTHOR                  Doc::alias("author")
#define COMMAND_PUBLISHER               Doc::alias("publisher")
#define COMMAND_COPYRYEAR               Doc::alias("copyryear")
#define COMMAND_COPYRHOLDER             Doc::alias("copyrholder")
#define COMMAND_PERMISSIONS             Doc::alias("permissions")
#define COMMAND_LIFECYCLEVERSION        Doc::alias("lifecycleversion")
#define COMMAND_LIFECYCLEWSTATUS        Doc::alias("lifecyclestatus")
#define COMMAND_LICENSEYEAR             Doc::alias("licenseyear")
#define COMMAND_LICENSENAME             Doc::alias("licensename")
#define COMMAND_LICENSEDESCRIPTION      Doc::alias("licensedescription")
#define COMMAND_RELEASEDATE             Doc::alias("releasedate")
#define COMMAND_QTVARIABLE              Doc::alias("qtvariable")

QT_END_NAMESPACE

#endif
