/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  tree.h
*/

#ifndef TREE_H
#define TREE_H

#include <QtCore/qstack.h>
#include "node.h"

QT_BEGIN_NAMESPACE

class QStringList;
class QDocDatabase;

struct TargetRec
{
  public:
    enum Type { Unknown, Target, Keyword, Contents, Class, Function, Page, Subtitle };

    TargetRec(const QString& name,
              const QString& title,
              TargetRec::Type type,
              Node* node,
              int priority)
    : node_(node), ref_(name), title_(title), priority_(priority), type_(type) { }

    bool isEmpty() const { return ref_.isEmpty(); }

    Node* node_;
    QString ref_;
    QString title_;
    int priority_;
    Type type_;
};

typedef QMultiMap<QString, TargetRec*> TargetMap;
typedef QMultiMap<QString, DocNode*> DocNodeMultiMap;
typedef QMap<QString, QmlClassNode*> QmlTypeMap;
typedef QMultiMap<QString, const ExampleNode*> ExampleNodeMap;

class Tree
{
 private:
    friend class QDocForest;
    friend class QDocDatabase;

    typedef QMap<PropertyNode::FunctionRole, QString> RoleMap;
    typedef QMap<PropertyNode*, RoleMap> PropertyMap;

    Tree(const QString& module, QDocDatabase* qdb);
    ~Tree();

    Node* findNodeForInclude(const QStringList& path) const;
    ClassNode* findClassNode(const QStringList& path, const Node* start = 0) const;
    NamespaceNode* findNamespaceNode(const QStringList& path) const;
    FunctionNode* findFunctionNode(const QStringList& parentPath, const FunctionNode* clone);
    const Node* findFunctionNode(const QString& target, const Node* relative, Node::Genus genus);

    Node* findNodeRecursive(const QStringList& path,
                            int pathIndex,
                            const Node* start,
                            Node::Type type) const;
    Node* findNodeRecursive(const QStringList& path,
                            int pathIndex,
                            Node* start,
                            const NodeTypeList& types) const;

    const Node* findNodeForTarget(const QStringList& path,
                                  const QString& target,
                                  const Node* node,
                                  int flags,
                                  Node::Genus genus,
                                  QString& ref) const;
    const Node* matchPathAndTarget(const QStringList& path,
                                   int idx,
                                   const QString& target,
                                   const Node* node,
                                   int flags,
                                   Node::Genus genus,
                                   QString& ref) const;

    const Node* findNode(const QStringList &path,
                         const Node* relative,     // = 0,
                         int findFlags,            // = 0,
                         Node::Genus genus) const; // = Node::DontCare) const;

    QmlClassNode* findQmlTypeNode(const QStringList& path);

    Node* findNodeByNameAndType(const QStringList& path, Node::Type type) const;
    InnerNode* findRelatesNode(const QStringList& path);
    QString getRef(const QString& target, const Node* node) const;
    void insertTarget(const QString& name,
                      const QString& title,
                      TargetRec::Type type,
                      Node* node,
                      int priority);
    void resolveTargets(InnerNode* root);
    const Node* findUnambiguousTarget(const QString& target, QString& ref) const;
    const DocNode* findDocNodeByTitle(const QString& title) const;

    void addPropertyFunction(PropertyNode *property,
                             const QString &funcName,
                             PropertyNode::FunctionRole funcRole);
    void resolveInheritance(InnerNode* n = 0);
    void resolveInheritanceHelper(int pass, ClassNode* cn);
    void resolveProperties();
    void resolveCppToQmlLinks();
    void fixInheritance(NamespaceNode *rootNode = 0);
    NamespaceNode *root() { return &root_; }

    const FunctionNode *findFunctionNode(const QStringList &path,
                                         const Node *relative = 0,
                                         int findFlags = 0,
                                         Node::Genus genus = Node::DontCare) const;
    const NamespaceNode *root() const { return &root_; }

    FunctionNode *findVirtualFunctionInBaseClasses(ClassNode *classe,
                                                   FunctionNode *clone);
    NodeList allBaseClasses(const ClassNode *classe) const;
    QString refForAtom(const Atom* atom);

    const CNMap& groups() const { return groups_; }
    const CNMap& modules() const { return modules_; }
    const CNMap& qmlModules() const { return qmlModules_; }
    const CNMap& getCollections(Node::Type t) const {
        if (t == Node::Group)
            return groups_;
        if (t == Node::Module)
            return modules_;
        return qmlModules_;
    }

    CollectionNode* getCorrespondingCollection(CollectionNode* cn);

    GroupNode* getGroup(const QString& name);
    ModuleNode* getModule(const QString& name);
    QmlModuleNode* getQmlModule(const QString& name);

    GroupNode* findGroup(const QString& name);
    ModuleNode* findModule(const QString& name);
    QmlModuleNode* findQmlModule(const QString& name);

    GroupNode* addGroup(const QString& name);
    ModuleNode* addModule(const QString& name);
    QmlModuleNode* addQmlModule(const QString& name);

    GroupNode* addToGroup(const QString& name, Node* node);
    ModuleNode* addToModule(const QString& name, Node* node);
    QmlModuleNode* addToQmlModule(const QString& name, Node* node);

    QmlClassNode* lookupQmlType(const QString& name) const { return qmlTypeMap_.value(name); }
    void insertQmlType(const QString& key, QmlClassNode* n);
    void addExampleNode(ExampleNode* n) { exampleNodeMap_.insert(n->title(), n); }
    ExampleNodeMap& exampleNodeMap() { return exampleNodeMap_; }
    const Node* checkForCollision(const QString& name);

 public:
    const QString& moduleName() const { return module_; }

private:
    QString module_;
    QDocDatabase* qdb_;
    NamespaceNode root_;
    PropertyMap unresolvedPropertyMap;
    DocNodeMultiMap         docNodesByTitle_;
    TargetMap               nodesByTargetRef_;
    TargetMap               nodesByTargetTitle_;
    CNMap                   groups_;
    CNMap                   modules_;
    CNMap                   qmlModules_;
    QmlTypeMap              qmlTypeMap_;
    ExampleNodeMap          exampleNodeMap_;
};

QT_END_NAMESPACE

#endif
